#include "vapch.hpp"
#include "VulkanRenderer.hpp"

#include "imgui.h"

#include "Vulkan.hpp"
#include "VulkanContext.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/SceneRenderer.hpp"

#include "VulkanPipeline.hpp"
#include "VulkanVertexBuffer.hpp"
#include "VulkanIndexBuffer.hpp"
#include "VulkanFramebuffer.hpp"
#include "VulkanRenderCommandBuffer.hpp"

#include "VulkanShader.hpp"
#include "VulkanTexture.hpp"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

#include "Core/Timer.hpp"
#include "Debug/Profiler.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace Vanta {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<Texture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		std::unordered_map<SceneRenderer*, std::vector<VulkanShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;

		int32_t SelectedDrawCall = -1;
		int32_t DrawCallCount = 0;
	};

	static VulkanRendererData* s_Data = nullptr;

	namespace Utils {

		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
				case 0x10DE: return "NVIDIA";
				case 0x1002: return "AMD";
				case 0x8086: return "INTEL";
				case 0x13B5: return "ARM";
			}
			return "Unknown";
		}

	}

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();
		const auto& config = Renderer::GetConfig();
		s_Data->DescriptorPools.resize(config.FramesInFlight);
		s_Data->DescriptorPoolAllocationCount.resize(config.FramesInFlight);

		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// Create descriptor pools
		Renderer::Submit([]() mutable
		{
			// Create Descriptor Pool
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 10000;
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]));
				s_Data->DescriptorPoolAllocationCount[i] = 0;
			}

		});

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.0f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.0f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.0f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.0f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		s_Data->BRDFLut = Renderer::GetBRDFLutTexture();
	}

	void VulkanRenderer::Shutdown()
	{
		VulkanShader::ClearUniformBuffers();
		delete s_Data;
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateUniformBufferWriteDescriptors(Ref<UniformBufferSet> uniformBufferSet, Ref<VulkanMaterial> vulkanMaterial)
	{
		VA_PROFILE_FUNC();

		size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
		if (s_Data->UniformBufferWriteDescriptorCache.find(uniformBufferSet.Raw()) != s_Data->UniformBufferWriteDescriptorCache.end())
		{
			const auto& shaderMap = s_Data->UniformBufferWriteDescriptorCache.at(uniformBufferSet.Raw());
			if (shaderMap.find(shaderHash) != shaderMap.end())
			{
				const auto& writeDescriptors = shaderMap.at(shaderHash);
				return writeDescriptors;
			}
		}

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Ref<VulkanShader> vulkanShader = vulkanMaterial->GetShader().As<VulkanShader>();
		if (vulkanShader->HasDescriptorSet(0))
		{
			const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
			if (!shaderDescriptorSets.empty())
			{
				for (auto&& [binding, shaderUB] : shaderDescriptorSets[0].UniformBuffers)
				{
					auto& writeDescriptors = s_Data->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
					writeDescriptors.resize(framesInFlight);
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						Ref<VulkanUniformBuffer> uniformBuffer = uniformBufferSet->Get(binding, 0, frame); // set = 0 for now

						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
						writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
						writeDescriptors[frame].push_back(writeDescriptorSet);
					}
				}
			
			}
		}

		return s_Data->UniformBufferWriteDescriptorCache[uniformBufferSet.Raw()][shaderHash];
	}

	static const std::vector<std::vector<VkWriteDescriptorSet>>& RT_RetrieveOrCreateStorageBufferWriteDescriptors(Ref<StorageBufferSet> storageBufferSet, Ref<VulkanMaterial> vulkanMaterial)
	{
		size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
		if (s_Data->StorageBufferWriteDescriptorCache.find(storageBufferSet.Raw()) != s_Data->StorageBufferWriteDescriptorCache.end())
		{
			const auto& shaderMap = s_Data->StorageBufferWriteDescriptorCache.at(storageBufferSet.Raw());
			if (shaderMap.find(shaderHash) != shaderMap.end())
			{
				const auto& writeDescriptors = shaderMap.at(shaderHash);
				return writeDescriptors;
			}
		}

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Ref<VulkanShader> vulkanShader = vulkanMaterial->GetShader().As<VulkanShader>();
		if (vulkanShader->HasDescriptorSet(0))
		{
			const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSets();
			if (!shaderDescriptorSets.empty())
			{
				for (auto&& [binding, shaderSB] : shaderDescriptorSets[0].StorageBuffers)
				{
					auto& writeDescriptors = s_Data->StorageBufferWriteDescriptorCache[storageBufferSet.Raw()][shaderHash];
					writeDescriptors.resize(framesInFlight);
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						Ref<VulkanStorageBuffer> storageBuffer = storageBufferSet->Get(binding, 0, frame); // set = 0 for now

						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						writeDescriptorSet.pBufferInfo = &storageBuffer->GetDescriptorBufferInfo();
						writeDescriptorSet.dstBinding = storageBuffer->GetBinding();
						writeDescriptors[frame].push_back(writeDescriptorSet);
					}
				}
			}
		}

		return s_Data->StorageBufferWriteDescriptorCache[storageBufferSet.Raw()][shaderHash];
	}

	void VulkanRenderer::RT_UpdateMaterialForRendering(Ref<VulkanMaterial> material, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet)
	{
		if (uniformBufferSet)
		{
			auto writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, material);
			if (storageBufferSet)
			{
				const auto& storageBufferWriteDescriptors = RT_RetrieveOrCreateStorageBufferWriteDescriptors(storageBufferSet, material);

				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t frame = 0; frame < framesInFlight; frame++)
				{
					writeDescriptors[frame].reserve(writeDescriptors[frame].size() + storageBufferWriteDescriptors[frame].size());
					writeDescriptors[frame].insert(writeDescriptors[frame].end(), storageBufferWriteDescriptors[frame].begin(), storageBufferWriteDescriptors[frame].end());
				}
			}
			material->RT_UpdateForRendering(writeDescriptors);
		}
		else
		{
			material->RT_UpdateForRendering();
		}
	}

	VkSampler VulkanRenderer::GetClampSampler()
	{
		if (s_Data->SamplerClamp)
			return s_Data->SamplerClamp;

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 100.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &s_Data->SamplerClamp));
	}

	int32_t& VulkanRenderer::GetSelectedDrawCall()
	{
		return s_Data->SelectedDrawCall;
	}

	void VulkanRenderer::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, transform]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::RenderMesh");
			VA_SCOPE_PERF("VulkanRenderer::RenderMesh");

			if (s_Data->SelectedDrawCall != -1 && s_Data->DrawCallCount > s_Data->SelectedDrawCall)
				return;

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
			Ref<VulkanVertexBuffer> vulkanMeshVB = meshAsset->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(meshAsset->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			std::vector<std::vector<VkWriteDescriptorSet>> writeDescriptors;

			auto& materials = mesh->GetMaterials();
			for (auto& material : materials)
			{
				Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
				RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);
			}

			const auto& meshAssetSubmeshes = meshAsset->GetSubmeshes();
			auto& submeshes = mesh->GetSubmeshes();
			for (uint32_t submeshIndex : submeshes)
			{
				if (s_Data->SelectedDrawCall != -1 && s_Data->DrawCallCount > s_Data->SelectedDrawCall)
					break;

				const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
				auto material = mesh->GetMaterials()[submesh.MaterialIndex].As<VulkanMaterial>();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				VkDescriptorSet descriptorSet = material->GetDescriptorSet(frameIndex);

				// NOTE: Descriptor Set 1 is owned by the renderer
				std::array<VkDescriptorSet, 2> descriptorSets = {
					descriptorSet,
					s_Data->ActiveRendererDescriptorSet
				};

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				glm::mat4 worldTransform = transform * submesh.Transform;

				Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				s_Data->DrawCallCount++;
			}
		});
	}

	void VulkanRenderer::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform, Buffer additionalUniforms)
	{
		VA_CORE_ASSERT(mesh);
		VA_CORE_ASSERT(mesh->GetMeshAsset());

		Buffer pushConstantBuffer;
		pushConstantBuffer.Allocate(sizeof(glm::mat4) + additionalUniforms.Size);
		if (additionalUniforms.Size)
			pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size, sizeof(glm::mat4));

		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, vulkanMaterial, transform, pushConstantBuffer]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::RenderMeshWithMaterial");
			VA_SCOPE_PERF("VulkanRenderer::RenderMeshWithMaterial");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
			auto vulkanMeshVB = meshAsset->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(meshAsset->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			float lineWidth = vulkanPipeline->GetSpecification().LineWidth;
			if (lineWidth != 1.0f)
				vkCmdSetLineWidth(commandBuffer, lineWidth);

			// Bind descriptor sets describing shader binding points
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantBuffer.Size, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			const auto& meshAssetSubmeshes = meshAsset->GetSubmeshes();
			auto& submeshes = mesh->GetSubmeshes();
			for (uint32_t submeshIndex : submeshes)
			{
				const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
				glm::mat4 worldTransform = transform * submesh.Transform;
				pushConstantBuffer.Write(&worldTransform, sizeof(glm::mat4));
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantBuffer.Size, pushConstantBuffer.Data);
				vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			}
			pushConstantBuffer.Release();
		});
	}

	void VulkanRenderer::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, transform]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::RenderQuad");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			
			RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount /*= 0*/)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		if (indexCount == 0)
			indexCount = indexBuffer->GetCount();

		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, vertexBuffer, indexBuffer, transform, indexCount]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::RenderGeometry");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = vertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = indexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			const auto& writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, vulkanMaterial);
			vulkanMaterial->RT_UpdateForRendering(writeDescriptors);

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		});
	}

	VkDescriptorSet VulkanRenderer::RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		VA_PROFILE_FUNC();

		uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
		allocInfo.descriptorPool = s_Data->DescriptorPools[bufferIndex];
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkDescriptorSet result;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));
		s_Data->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
		return result;
	}

#if 0
	void VulkanRenderer::SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t set)
	{
		Renderer::Submit([uniformBuffer, set]()
		{


			VA_CORE_ASSERT(set == 1); // Currently we only bind to Renderer-maintaned UBs, which are in descriptor set 1

			Ref<VulkanUniformBuffer> vulkanUniformBuffer = uniformBuffer.As<VulkanUniformBuffer>();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pBufferInfo = &vulkanUniformBuffer->GetDescriptorBufferInfo();
			writeDescriptorSet.dstBinding = uniformBuffer->GetBinding();
			writeDescriptorSet.dstSet = s_Data->RendererDescriptorSet.DescriptorSets[0];

			VA_CORE_WARN("VulkanRenderer - Updating descriptor set (VulkanRenderer::SetUniformBuffer)");
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		});
	}
#endif

	/* void VulkanRenderer::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> pipelineCompute, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
	{
		auto vulkanMaterial = material.As<VulkanMaterial>();
		auto pipeline = pipelineCompute.As<VulkanComputePipeline>();
		Renderer::Submit([renderCommandBuffer, pipeline, vulkanMaterial, uniformBufferSet, storageBufferSet, screenSize, workGroups]() mutable
		{
			const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

			const VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);

			pipeline->Begin(renderCommandBuffer);
			pipeline->SetPushConstants(glm::value_ptr(screenSize), sizeof(glm::ivec2));
			pipeline->Dispatch(descriptorSet, workGroups.x, workGroups.y, workGroups.z);
			pipeline->End();
		});
	} */

	void VulkanRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
		SubmitFullscreenQuad(renderCommandBuffer, pipeline, uniformBufferSet, nullptr, material);
	}

	void VulkanRenderer::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet,
		Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::SubmitFullscreenQuad");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			RT_UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet, storageBufferSet);

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer.Size)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		Buffer vertexPushConstantBuffer;
		if (vertexShaderOverrides)
		{
			vertexPushConstantBuffer.Allocate(vertexShaderOverrides.Size);
			vertexPushConstantBuffer.Write(vertexShaderOverrides.Data, vertexShaderOverrides.Size);
		}

		Buffer fragmentPushConstantBuffer;
		if (fragmentPushConstantBuffer)
		{
			fragmentPushConstantBuffer.Allocate(fragmentShaderOverrides.Size);
			fragmentPushConstantBuffer.Write(fragmentShaderOverrides.Data, fragmentShaderOverrides.Size);
		}

		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, vulkanMaterial, vertexPushConstantBuffer, fragmentPushConstantBuffer]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::SubmitFullscreenQuad");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			if (uniformBufferSet)
			{
				const auto& writeDescriptors = RT_RetrieveOrCreateUniformBufferWriteDescriptors(uniformBufferSet, vulkanMaterial);
				vulkanMaterial->RT_UpdateForRendering(writeDescriptors);
			}
			else
			{
				vulkanMaterial->RT_UpdateForRendering();
			}

			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
			VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(bufferIndex);
			if (descriptorSet)
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

			if (vertexPushConstantBuffer)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, vertexPushConstantBuffer.Size, vertexPushConstantBuffer.Data);
			if (fragmentPushConstantBuffer)
				vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, vertexPushConstantBuffer.Size, fragmentPushConstantBuffer.Size, fragmentPushConstantBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<Image2D> shadow)
	{
		if (!environment)
			environment = Renderer::GetEmptyEnvironment();

		Renderer::Submit([sceneRenderer, environment, shadow]() mutable
		{
			VA_PROFILE_FUNC("VulkanRenderer::SetSceneEnvironment");
			
			auto shader = Renderer::GetShaderLibrary()->Get("VantaPBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();
			uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();

			if (s_Data->RendererDescriptorSet.find(sceneRenderer.Raw()) == s_Data->RendererDescriptorSet.end())
			{
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				s_Data->RendererDescriptorSet[sceneRenderer.Raw()].resize(framesInFlight);
				for (uint32_t i = 0; i < framesInFlight; i++)
					s_Data->RendererDescriptorSet.at(sceneRenderer.Raw())[i] = pbrShader->CreateDescriptorSets(1);

			}

			VkDescriptorSet descriptorSet = s_Data->RendererDescriptorSet.at(sceneRenderer.Raw())[bufferIndex].DescriptorSets[0];
			s_Data->ActiveRendererDescriptorSet = descriptorSet;

			std::array<VkWriteDescriptorSet, 4> writeDescriptors;

			Ref<VulkanTextureCube> radianceMap = environment->RadianceMap.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceMap = environment->IrradianceMap.As<VulkanTextureCube>();
		
			writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", 1);
			writeDescriptors[0].dstSet = descriptorSet;
			const auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

			writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", 1);
			writeDescriptors[1].dstSet = descriptorSet;
			const auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

			writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", 1);
			writeDescriptors[2].dstSet = descriptorSet;
			const auto& brdfLutImageInfo = s_Data->BRDFLut.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
			writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

			writeDescriptors[3] = *pbrShader->GetDescriptorSet("u_ShadowMapTexture", 1);
			writeDescriptors[3].dstSet = descriptorSet;
			const auto& shadowImageInfo = shadow.As<VulkanImage2D>()->GetDescriptor();
			writeDescriptors[3].pImageInfo = &shadowImageInfo;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		});
	}

	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([]()
		{
			VA_PROFILE_FUNC("VulkanRenderer::BeginFrame");

			VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();

			// Reset descriptor pools here
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();
			vkResetDescriptorPool(device, s_Data->DescriptorPools[bufferIndex], 0);
			memset(s_Data->DescriptorPoolAllocationCount.data(), 0, s_Data->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));

			s_Data->DrawCallCount = 0;

#if 0
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			commandBuffer = drawCommandBuffer;
			VA_CORE_ASSERT(commandBuffer);
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));
#endif
		});
	}

	void VulkanRenderer::EndFrame()
	{
#if 0
		Renderer::Submit([]()
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
			commandBuffer = nullptr;
		});
#endif
	}

	void VulkanRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, const Ref<RenderPass>& renderPass, bool explicitClear)
	{
		Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
		{
			VA_PROFILE_FUNC("VulkanRenderer::BeginRenderPass");
			
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			auto fb = renderPass->GetSpecification().TargetFramebuffer;
			Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
			const auto& fbSpec = framebuffer->GetSpecification();

			uint32_t width = framebuffer->GetWidth();
			uint32_t height = framebuffer->GetHeight();

			VkViewport viewport = {};
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			if (framebuffer->GetSpecification().SwapChainTarget)
			{
				VulkanSwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
				width = swapChain.GetWidth();
				height = swapChain.GetHeight();
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

				viewport.x = 0.0f;
				viewport.y = (float)height;
				viewport.width = (float)width;
				viewport.height = -(float)height;
			}
			else
			{
				width = framebuffer->GetWidth();
				height = framebuffer->GetHeight();
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = (float)width;
				viewport.height = (float)height;
			}

			// TODO: Does our framebuffer have a depth attachment?
			const auto& clearValues = framebuffer->GetVulkanClearValues();
			renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			if (explicitClear)
			{
				uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
				uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
				VA_CORE_ASSERT(clearValues.size() == totalAttachmentCount);

				std::vector<VkClearAttachment> attachments(totalAttachmentCount);
				std::vector<VkClearRect> clearRects(totalAttachmentCount);
				for (uint32_t i = 0; i < colorAttachmentCount; i++)
				{
					attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					attachments[i].colorAttachment = i;
					attachments[i].clearValue = clearValues[i];

					clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
					clearRects[i].rect.extent = { width, height };
					clearRects[i].baseArrayLayer = 0;
					clearRects[i].layerCount = 1;
				}

				if (framebuffer->HasDepthAttachment())
				{
					attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
					clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
					clearRects[colorAttachmentCount].rect.extent = { width, height };
					clearRects[colorAttachmentCount].baseArrayLayer = 0;
					clearRects[colorAttachmentCount].layerCount = 1;
				}

				vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());

			}

			// Update dynamic viewport state
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		});
	}

	void VulkanRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		Renderer::Submit([renderCommandBuffer]()
		{
			VA_PROFILE_FUNC("VulkanRenderer::EndRenderPass");

			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);

			vkCmdEndRenderPass(commandBuffer);
		});
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> VulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		if (!Renderer::GetConfig().ComputeEnvironmentMaps)
			return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
		VA_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");

		Ref<TextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		Ref<TextureCube> envFiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		
		// Convert equirectangular to cubemap
		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		Ref<VulkanComputePipeline> equirectangularConversionPipeline = Ref<VulkanComputePipeline>::Create(equirectangularConversionShader);

		Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = equirectangularConversionPipeline->GetShader();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			Ref<VulkanTexture2D> envEquirectVK = envEquirect.As<VulkanTexture2D>();
			writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

			envUnfilteredCubemap->GenerateMips(true);
		});

		Ref<Shader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		Ref<VulkanComputePipeline> environmentMipFilterPipeline = Ref<VulkanComputePipeline>::Create(environmentMipFilterShader);

		Renderer::Submit([environmentMipFilterPipeline, envUnfiltered, envFiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentMipFilterPipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

			uint32_t mipCount = Utils::CalculateMipCount(cubemapSize, cubemapSize);

			std::vector<VkWriteDescriptorSet> writeDescriptors(mipCount * 2);
			std::vector<VkDescriptorImageInfo> mipImageInfos(mipCount);
			auto descriptorSet = shader->CreateDescriptorSets(0, 12);
			for (uint32_t i = 0; i < mipCount; i++)
			{
				VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
				mipImageInfo = imageInfo;
				mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);

				writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
				writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

				Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
				writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
				writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
			}

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			environmentMipFilterPipeline->Begin(); // begin compute pass
			const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 32);
				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);
				environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
				environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
			}
			environmentMipFilterPipeline->End();
		});

		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		Ref<VulkanComputePipeline> environmentIrradiancePipeline = Ref<VulkanComputePipeline>::Create(environmentIrradianceShader);
		Ref<TextureCube> irradianceMap = TextureCube::Create(ImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize);

		Renderer::Submit([environmentIrradiancePipeline, irradianceMap, envFiltered]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentIrradiancePipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceCubemap = irradianceMap.As<VulkanTextureCube>();
			auto descriptorSet = shader->CreateDescriptorSets();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

			writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			environmentIrradiancePipeline->Begin();
			environmentIrradiancePipeline->SetPushConstants(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t));
			environmentIrradiancePipeline->Dispatch(descriptorSet.DescriptorSets[0], irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			environmentIrradiancePipeline->End();

			irradianceCubemap->GenerateMips();
		});

		return { envFiltered, irradianceMap };
	}

	Ref<TextureCube> VulkanRenderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<TextureCube> environmentMap = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);

		Ref<Shader> preethamSkyShader = Renderer::GetShaderLibrary()->Get("PreethamSky");
		Ref<VulkanComputePipeline> preethamSkyComputePipeline = Ref<VulkanComputePipeline>::Create(preethamSkyShader);

		glm::vec3 params = { turbidity, azimuth, inclination };
		Renderer::Submit([preethamSkyComputePipeline, environmentMap, cubemapSize, params]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = preethamSkyComputePipeline->GetShader();

			std::array<VkWriteDescriptorSet, 1> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = environmentMap.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			preethamSkyComputePipeline->Begin();
			preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
			preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
			preethamSkyComputePipeline->End();

			envUnfilteredCubemap->GenerateMips(true);
		});

		return environmentMap;
	}

}

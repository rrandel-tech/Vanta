va_log("test.lua loaded successfully!")

local values = { 1, 2, 3, 4, 5 }
local sum = 0
for _, v in ipairs(values) do
    sum = sum + v
end

va_log("Sum of {1,2,3,4,5} = " .. tostring(sum))
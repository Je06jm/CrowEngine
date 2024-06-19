#ifndef CROW_SHADER_HPP
#define CROW_SHADER_HPP

#include <Crow/Vulkan.hpp>

#include <optional>
#include <string>
#include <vector>

namespace crow {

std::vector<uint32_t> CompileGLSLShader(const std::string& file_name,
                                        const std::string& code,
                                        VkShaderStageFlagBits stage,
                                        const std::string& include_path,
                                        bool gen_debug = false);

std::optional<VkShaderModule>
CreateShaderFromSPIR_V(const std::vector<uint32_t>& spir_v);

} // namespace crow

#endif
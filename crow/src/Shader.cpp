#include <Crow/Shader.hpp>

#include <shaderc/shaderc.h>

#include <Crow/Log.hpp>

namespace crow {

std::vector<uint32_t> CompileGLSLShader(const std::string& file_name,
                                        const std::string& code,
                                        VkShaderStageFlagBits stage,
                                        const std::string& include_path,
                                        bool gen_debug) {
    shaderc_shader_kind shader_kind;
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        shader_kind = shaderc_glsl_default_vertex_shader;
        break;

    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        shader_kind = shaderc_glsl_default_tess_control_shader;
        break;

    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        shader_kind = shaderc_glsl_default_tess_evaluation_shader;
        break;

    case VK_SHADER_STAGE_GEOMETRY_BIT:
        shader_kind = shaderc_glsl_default_geometry_shader;
        break;

    case VK_SHADER_STAGE_FRAGMENT_BIT:
        shader_kind = shaderc_glsl_default_fragment_shader;
        break;

    case VK_SHADER_STAGE_COMPUTE_BIT:
        shader_kind = shaderc_glsl_default_compute_shader;
        break;

    case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
        shader_kind = shaderc_glsl_default_raygen_shader;
        break;

    case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
        shader_kind = shaderc_glsl_default_anyhit_shader;
        break;

    case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
        shader_kind = shaderc_glsl_default_closesthit_shader;
        break;

    case VK_SHADER_STAGE_MISS_BIT_KHR:
        shader_kind = shaderc_glsl_default_miss_shader;
        break;

    case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
        shader_kind = shaderc_glsl_default_intersection_shader;
        break;

    case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
        shader_kind = shaderc_glsl_default_callable_shader;
        break;

    case VK_SHADER_STAGE_TASK_BIT_EXT:
        shader_kind = shaderc_glsl_default_task_shader;
        break;

    case VK_SHADER_STAGE_MESH_BIT_EXT:
        shader_kind = shaderc_glsl_default_mesh_shader;
        break;

    default:
        println("Unknown shader type {}", int(stage));
        return {};
    }

    auto compiler = shaderc_compiler_initialize();
    auto options = shaderc_compile_options_initialize();

    if (gen_debug) {
        shaderc_compile_options_add_macro_definition(options, "DEBUG", 5, NULL,
                                                     0);
        shaderc_compile_options_set_generate_debug_info(options);
    }

    shaderc_compile_options_set_optimization_level(
        options, shaderc_optimization_level_performance);

    std::vector<char> path;
    path.resize(include_path.size() + 1);
    for (size_t i = 0; i < include_path.size() + 1; i++) {
        path[i] = include_path[i];
    }

    shaderc_compile_options_set_include_callbacks(
        options,
        [](void* user_data, const char* requested_source, int type,
           const char* requesting_source,
           size_t include_depth) -> shaderc_include_result* {
            // TODO Resolve include lookup
        },

        [](void* user_data, shaderc_include_result* include_result) -> void {
            // TODO Delete include lookup
        },
        static_cast<void*>(path.data()));

    shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan,
                                           shaderc_env_version_vulkan_1_2);

    shaderc_compile_options_set_auto_bind_uniforms(options, false);
    shaderc_compile_options_set_auto_combined_image_sampler(options, false);

    shaderc_compile_options_set_preserve_bindings(options, false);

    shaderc_compile_options_set_auto_map_locations(options, false);

    shaderc_compile_options_set_vulkan_rules_relaxed(options, false);

    shaderc_compile_options_set_invert_y(options, false);

    shaderc_compile_options_set_nan_clamp(options, true);

    auto result = shaderc_compile_into_spv(compiler, code.c_str(), code.size(),
                                           shader_kind, file_name.c_str(),
                                           "main", options);

    auto status = shaderc_result_get_compilation_status(result);

    std::vector<uint32_t> data;

    switch (status) {
    case shaderc_compilation_status_compilation_error:
        println("{} compilation failure: {}", file_name,
                shaderc_result_get_error_message(result));
        break;

    case shaderc_compilation_status_configuration_error:
        println("{} configuration failure: {}", file_name,
                shaderc_result_get_error_message(result));
        break;

    case shaderc_compilation_status_internal_error:
        println("{} internal failure: {}", file_name,
                shaderc_result_get_error_message(result));
        break;

    case shaderc_compilation_status_transformation_error:
        println("{} transformation error: {}", file_name,
                shaderc_result_get_error_message(result));
        break;

    case shaderc_compilation_status_validation_error:
        println("{} validation error: {}", file_name,
                shaderc_result_get_error_message(result));
        break;

    case shaderc_compilation_status_success: {
        size_t length = shaderc_result_get_length(result);
        auto bytes = shaderc_result_get_bytes(result);

        data.resize(length / 4);
        auto words = reinterpret_cast<const uint32_t*>(bytes);
        for (size_t i = 0; i < length / 4; i++) {
            data[i] = words[i];
        }

        break;
    }

    default:
        println("{} unknown error: {}", file_name,
                shaderc_result_get_error_message(result));
        break;
    }

    shaderc_result_release(result);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    return data;
}

std::optional<VkShaderModule>
CreateShaderFromSPIR_V(const std::vector<uint32_t>& spir_v) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = spir_v.size() * 4;
    create_info.pCode = static_cast<const uint32_t*>(spir_v.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(vkb_device.device, &create_info,
                             vkb_device.allocation_callbacks,
                             &shader_module) != VK_SUCCESS) {
        println("Could not create shader module");
        return {};
    }

    return shader_module;
}

} // namespace crow
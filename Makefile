VULKAN_SDK = C:\VulkanSDK\1.3.283.0
export VULKAN_SDK

ENGINE_LIBRARY = libcrowengine.a
EDITOR = croweditor

# Shamlessly stolen from https://stackoverflow.com/questions/2483182/recursive-wildcards-in-gnu-make
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

CXX = g++

CXX_COMMON_INCLUDES = -ICrow/include

CXX_ENGINE_INCLUDES += -ICrow/thirdparties/vk-bootstrap/src
CXX_ENGINE_INCLUDES += -ICrow/thirdparties/VulkanMemoryAllocator/include
CXX_ENGINE_INCLUDES += -I$(VULKAN_SDK)/Include

CXX_FLAGS = -std=c++23 -Wall -Wextra $(CXX_COMMON_INCLUDES)

CXX_THIRDPARTIES_FLAG = -std=c++23 $(CXX_COMMON_INCLUDES)

CXX_ENGINE_SRCS = $(wildcard Crow/src/*.cpp)
CXX_ENGINE_OBJS = $(patsubst Crow/src/%.cpp,build/Crow/%.o,$(CXX_ENGINE_SRCS))
CXX_ENGINE_DEPS = $(patsubst Crow/src/%.cpp,build/Crow/%.d,$(CXX_ENGINE_SRCS))

CXX_VK_BOOTSTRAP_SRCS = $(wildcard Crow/thirdparties/vk-bootstrap/src/*.cpp)
CXX_VK_BOOTSTRAP_OBJS = $(patsubst Crow/thirdparties/vk-bootstrap/src/%.cpp,build/thirdparties/vk-bootstrap/%.o,$(CXX_VK_BOOTSTRAP_SRCS))

CXX_EDITOR_SRCS = $(wildcard Editor/src/*.cpp)
CXX_EDITOR_OBJS = $(patsubst Editor/src/%.cpp,build/Editor/%.o,$(CXX_EDITOR_SRCS))
CXX_EDITOR_DEPS = $(patsubst Editor/src/%.cpp,build/Editor/%.d,$(CXX_EDITOR_SRCS))

AR = ar
AR_FLAGS = rcs

LD = g++

LD_FLAGS = -LCrow/libs -L$(VULKAN_SDK)/Lib
LD_FLAGS += -lgdi32 -luser32 -lkernel32 -lshell32 -lvulkan-1 -lshaderc -lstdc++exp

-include $(CXX_ENGINE_DEPS)

setuproj:
	@-mkdir bin/
	@-mkdir build/
	@-mkdir build/Crow
	@-mkdir build/Editor
	@-mkdir build/thirdparties
	@-mkdir build/thirdparties/vk-bootstrap

build/Crow/%.o: Crow/src/%.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_ENGINE_INCLUDES) -MMD -MP -c $< -o $@

build/thirdparties/vk-bootstrap/%.o: Crow/thirdparties/vk-bootstrap/src/%.cpp
	$(CXX) $(CXX_THIRDPARTIES_FLAG) $(CXX_ENGINE_INCLUDES) -c $< -o $@

engine_objs: $(CXX_ENGINE_OBJS) $(CXX_VK_BOOTSTRAP_OBJS)
	$(AR) $(AR_FLAGS) bin/$(ENGINE_LIBRARY) $(CXX_ENGINE_OBJS) $(CXX_VK_BOOTSTRAP_OBJS)

engine: CXX_FLAGS += -O2
engine: engine_objs

engine_debug: CXX_FLAGS += -g
engine_debug: CXX_FLAGS += -DCROW_DEBUG
engine_debug: engine_objs

build/Editor/%.o: Editor/src/%.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_ENGINE_INCLUDES) -MMD -MP -c $< -o $@

editor: CXX_FLAGS += -O2
editor: engine_objs $(CXX_EDITOR_OBJS)
	$(LD) $(CXX_EDITOR_OBJS) bin/$(ENGINE_LIBRARY) Crow/libs/libglfw3.a $(LD_FLAGS) -o bin/$(EDITOR)

editor_debug: CXX_FLAGS += -g
editor_debug: CXX_FLAGS += -DCROW_DEBUG
editor_debug: engine_objs $(CXX_EDITOR_OBJS)
	$(LD) $(CXX_EDITOR_OBJS) bin/$(ENGINE_LIBRARY) Crow/libs/libglfw3.a $(LD_FLAGS) -o bin/$(EDITOR)

OBJS_TO_CLEAN = $(call rwildcard,build,*.o) $(call rwildcard,bin,*)

clean:
	@-rm $(OBJS_TO_CLEAN)

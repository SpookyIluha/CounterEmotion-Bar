BUILD_DIR=build
T3D_INST=$(shell realpath ../..)

include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

N64_CFLAGS += -std=gnu2x

src = main.c

assets_png = $(wildcard assets/*.png)
assets_ui = $(wildcard assets/UI/*.png)
assets_gltf = $(wildcard assets/*.glb)
assets_ttf = $(wildcard assets/*.ttf)
SOUND_LIST  = $(shell find assets/sfx/ -type f -name '*.wav')
MUSIC_LIST  = $(shell find assets/music/ -type f -name '*.wav')
assets_conv = $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite))) \
			  $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
			  $(addprefix filesystem/,$(notdir $(assets_gltf:%.glb=%.t3dm))) \
			  $(addprefix filesystem/UI/,$(notdir $(assets_ui:%.png=%.sprite))) \
			  $(addprefix filesystem/sfx/,$(notdir $(SOUND_LIST:%.wav=%.wav64))) \
			  $(addprefix filesystem/music/,$(notdir $(MUSIC_LIST:%.wav=%.wav64)))

all: counteremotion_bar.z64

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/UI/%.sprite: assets/UI/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) --dither ORDERED -o filesystem/UI "$<"

filesystem/%.t3dm: assets/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@
	$(N64_BINDIR)/mkasset -c 2 -o filesystem $@

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) --size 14 --outline 2 --range 20-7F --range 400-4FF -o $(dir $@) "$<"

filesystem/sfx/%.wav64: assets/sfx/%.wav
	@mkdir -p $(dir $@)
	@echo "    [SFX] $@"
	$(N64_AUDIOCONV) --wav-compress 1 --wav-resample 28000 -o $(dir $@) "$<"

filesystem/music/%.wav64: assets/music/%.wav
	@mkdir -p $(dir $@)
	@echo "    [MUSIC] $@"
	$(N64_AUDIOCONV) --wav-compress 1,bits=3 --wav-resample 16000 --wav-mono -o $(dir $@) "$<"

$(BUILD_DIR)/counteremotion_bar.dfs: $(assets_conv)
$(BUILD_DIR)/counteremotion_bar.elf: $(src:%.c=$(BUILD_DIR)/%.o)

counteremotion_bar.z64: N64_ROM_TITLE="Counteremotion Bar"
counteremotion_bar.z64: $(BUILD_DIR)/counteremotion_bar.dfs

clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf filesystem

build_lib:
	rm -rf $(BUILD_DIR) *.z64
	make -C $(T3D_INST)
	make all

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean

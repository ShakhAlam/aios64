x86_64_asm_source_files := $(shell find src/impl/x86_64 -name *.asm)

x86_64_asm_object_files := $(patsubst src/impl/x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

$(x86_64_asm_object_files): build/x86_64/%.o : src/impl/x86_64/%.asm
	$(info $<)
	$(info Source files: $(x86_64_asm_source_files))
	$(info Target files: $(x86_64_asm_object_files))
	$(info Building object $@) 
	$(info From $(patsubst build/x86_64/%.o, src/impl/x86_64/boot/%.asm, $@)) 
	mkdir -p $(dir $@) && \
	nasm -w+number-overflow -f elf64 $(patsubst build/x86_64/%.o, src/impl/x86_64/%.asm, $@) -o $@

all: .PHONY

.PHONY: build-x86_64
build-x86_64: $(x86_64_asm_object_files)
	mkdir -p dist/x86_64 && \
	x86_64-elf-ld -n -o dist/x86_64/kernel.bin -T targets/x86_64/linker.ld $(x86_64_asm_object_files) && \
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin && \
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel.iso targets/x86_64/iso


clean:
	rm $(x86_64_asm_object_files) 

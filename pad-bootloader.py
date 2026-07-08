Import("env")

def pad_bin(source, target, env):
    bin_path = str(target[0])
    BOOTLOADER_SIZE = 0x2000
    BOOTLOADER_FILE = bin_path

    with open(BOOTLOADER_FILE,"rb") as f : 
        raw_file = f.read()

    bytes_to_pad = BOOTLOADER_SIZE - len(raw_file)
    padding  = bytes([0xff for _ in range(bytes_to_pad)])

    with open(BOOTLOADER_FILE,"wb") as f:
        f.write(raw_file + padding)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin",pad_bin)
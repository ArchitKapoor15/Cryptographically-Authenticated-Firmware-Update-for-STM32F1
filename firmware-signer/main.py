import sys
import os
import subprocess
import struct

tb_signed_filename = "fw_tb_signed.bin"
encrypted_filename = "encrypted_image.bin"
signed_filename    = "signed_firmware.bin"

AES_KEY = "000102030405060708090a0b0c0d0e0f"
ZERO_IV = "00000000000000000000000000000000"

AES_BLOCK_SIZE  = 16
BOOTLOADER_SIZE = 0x2000
FW_INFO_OFFSET  = 0x150
FW_SIGN_OFFSET  = FW_INFO_OFFSET + AES_BLOCK_SIZE

FW_INFO_VERSION_OFFSET = 8
FW_INFO_LENGTH_OFFSET = 12

if len(sys.argv) < 3:
    print("Format : firmware-signer.py <input file> <Version number (in hex)>")
    exit(1)

with open(sys.argv[1],"rb") as f:
    f.seek(BOOTLOADER_SIZE)
    fw_image = bytearray(f.read())
    f.close()

Version = int(sys.argv[2], base=16)
struct.pack_into("<I",fw_image,(FW_INFO_OFFSET+FW_INFO_VERSION_OFFSET),Version)
struct.pack_into("<I",fw_image,(FW_INFO_OFFSET+FW_INFO_LENGTH_OFFSET),len(fw_image))

sign_img = fw_image[FW_INFO_OFFSET : FW_INFO_OFFSET + AES_BLOCK_SIZE]
sign_img += fw_image[:FW_INFO_OFFSET]
sign_img += fw_image[FW_INFO_OFFSET + AES_BLOCK_SIZE*2 : ]

with open(tb_signed_filename , "wb") as f:
    f.write(sign_img)
    f.close()

openssl_command = f"openssl enc -aes-128-cbc -nosalt -K {AES_KEY} -iv {ZERO_IV} -in {tb_signed_filename} -out {encrypted_filename}"
subprocess.call(openssl_command.split(" "))

with open(encrypted_filename,"rb") as f:
    f.seek(-AES_BLOCK_SIZE,os.SEEK_END)
    signature = f.read()
    f.close()

signature_text = ""
for byte in signature:
    signature_text += f"{byte:02x}"

print(f"Firmware Version {Version}")
print(f"Key = {AES_KEY}")
print(f"Signature = {signature_text}")

os.remove(tb_signed_filename)
os.remove(encrypted_filename)

fw_image[FW_SIGN_OFFSET:FW_SIGN_OFFSET+AES_BLOCK_SIZE] = signature

with open(signed_filename,"wb") as f:
    f.write(fw_image)
    f.close()
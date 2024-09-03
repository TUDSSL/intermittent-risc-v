with open('small.pcm', 'rb') as f:
    ba = bytearray(f.read())
    with open('data_pcm.h', 'w') as f2:
        f2.write(f"#define PCM_DATA_LENGTH {len(ba)}\n")
        f2.write("static const unsigned char pcm_data[] = {")
        for byte in ba:
            f2.write(str(int(byte)))
            f2.write(",")
        f2.write("};")

with open('small.adpcm', 'rb') as f:
    ba = bytearray(f.read())
    with open('data_adpcm.h', 'w') as f2:
        f2.write(f"#define ADPCM_DATA_LENGTH {len(ba)}\n")
        f2.write("static const unsigned char adpcm_data[] = {")
        for byte in ba:
            f2.write(str(int(byte)))
            f2.write(",")
        f2.write("};")
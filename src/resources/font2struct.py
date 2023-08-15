from PIL import Image

im = Image.open("./font.png")
px = im.load()
px_data = im.getdata()
max_elements_per_line = im.width

print("uint8_t VGA437_data[", im.width * im.height, "] = {")
print("\t", end="")
line_chars = 0

for px in px_data:
    line_chars += 1
    print((1 - px), end=",")
    if(line_chars > max_elements_per_line):
        line_chars = 0
        print("")
        print("\t", end="")

print("};")

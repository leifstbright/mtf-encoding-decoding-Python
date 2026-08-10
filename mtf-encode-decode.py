import sys
import fileinput
import os


count = 1
place_list = []

# adjusts the order of the words in the list
def adjust(w):
    global place_list
    #index = pre - 129
    place_list.insert(0, w)  # move to front

# finds the word in the list and reorganizes the list
def find_and_decode(pla):
    global place_list
    index = pla - 128  # positions start at 0
    w = place_list[index]
    place_list.remove(w)
    place_list.insert(0,w)
    return w        

# gets the word from the dictionary and reorganizes it
def encode(w,d,o):
    global count
    if not d:
        d[w] = 0
        count +=1
    else:
        if w in d:
            o = d[w]
            for key in d:
                if key != w and d[key] < o:
                    d[key] +=1
            d[w] = 0
        else:
            for key in d:
                d[key] +=1
            d[w] = 0
            count +=1
    return o

def encode_main():
    word_dict = {}
    file = sys.argv[1]
    magic_numbers = [0xba, 0x5e, 0xba, 0x11]
    base_name = os.path.splitext(os.path.basename(file))[0]
    output = f"{base_name}.mtf"
    with open(output, 'wb') as binary_file:
        binary_file.write(bytes(magic_numbers))
        with open(file, 'r') as f:
            for line in f:
                wordLine = line.split()
                for word in wordLine:
                    output = word
                    output = encode(word, word_dict, output)
                    if isinstance(output, int):
                        output_write = output + 128
                        output_bytes = output_write.to_bytes(1,byteorder = 'little')
                        binary_file.write(output_bytes)
                    else:
                        count_write = count + 127
                        count_bytes = count_write.to_bytes(1, byteorder = 'little')
                        binary_file.write(count_bytes)
                        for char in output:
                            binary_file.write(bytes([ord(char)]))
    
                binary_file.write(0x0a.to_bytes(1, byteorder = 'little'))                        
    f.close()

def decode_main():
    bin_file = sys.argv[1]
    word = ""
    line_words = []
    place = -1
    prev_char = 0
    base_name = os.path.splitext(os.path.basename(bin_file))[0]

    # Create the output file name by replacing .mtf with .txt
    output = f"{base_name}.txt"
    print(f"writing to {output}")
    with open(output, 'w') as t:
        print(f"Writing to {output}")
        with open(bin_file, 'rb') as b:
            b.read(4)  # skip the 4-byte magic header (only appears at the start)
            for line in b:
                for current in line:

                    if current == 0x0a:
                        if word != "":
                            line_words.append(word)
                            adjust(word)
                            word = ""
                        if line_words:
                            t.write(' '.join(line_words) + '\n')
                        elif prev_char == 0x0a:
                            t.write('\n')
                        line_words = []
                        place = -1

                    elif current < 0x80:
                        word += chr(current)

                    elif(current >= 0x80):

                        if word != "":
                            line_words.append(word)
                            adjust(word)
                            word = ""

                        if current < 128 + len(place_list):
                            line_words.append(find_and_decode(current))
                            place = -1
                        else:
                            place = current
                    prev_char = current
    if word:
        line_words.append(word)
        adjust(word)
    print(os.getcwd())


command = os.path.basename(__file__)
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {command} <file>")
        sys.exit(1)
    if sys.argv[1].lower().endswith('.mtf'):
        decode_main()
    else:
        encode_main()

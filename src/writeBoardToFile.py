import math

try:
    input = raw_input
except NameError:
    pass

def main():
    # parse copy pasted board output
    print("enter board output: ")
    prettyBoard = ""
    while (True):
        inStr = input().strip()
        if (inStr.strip() == ""):
            break
        if (inStr[0] == '-'):
            continue
        prettyBoard += inStr
    prettyBoard = prettyBoard.replace('] [','][').replace(' ','0').replace('[','|').replace(']','|').replace('00','0')
    
    # extract the integer values, with spaces replaced with 0s
    outLines = []
    for i in prettyBoard.split('|'):
        try:
            outLines.append(str(int(i)))
        except:
            pass
    
    # write the boardSize, followed by the newline separated contents of each row to the default output file "boardFile.txt"
    with open("boardFile.txt", "a") as f:
        #f.write(str(math.sqrt(len(outLines)))[:-2]+' ')
        for i in outLines:
            f.write(i+' ')
       
if __name__ == "__main__":
    main()
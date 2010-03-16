""".proto tp NUbot Model convertor

Converts .Proto webots files into a simpler NUbots model file.

Usage: python protoConvertor.py [options] [source]

Options:
  -i ..., --input=...     input .proto file
  -o ..., --output=...    output file
  -h, --help              show this help
  

Examples:
  protoConvertor.py -i input.proto -o output.file       
"""


import sys
import getopt
import copy

def usage():
    print __doc__
    
def ReadModelData(file, model, variables):
    currVariables = copy.copy(variables) # Don't want to make changed to the existing dict.
    KEYWORDS = ("field", "IS", "DEF",  "{",  "}",  "point",  "coordIndex")
    while 1:
        line = file.readline()
        if not line:
            print "End of File."
            break
        for word in line.split():
            if(word in KEYWORDS):
                data = [x.strip() for x in line.split()]
                if(word == KEYWORDS[0]): # field
                    # Extract field data and place in variables dict.
                    if(len(data) >= 4 and data[0] == "field"):
                        type = data[1]
                        name = data[2]
                        values = data[3:len(data)]
                        variables[name] = values
                        break
                elif(word == KEYWORDS[1]): # IS
                    if(len(data)>=3 and data[1] == "IS"):
                        newProperty = data[0]
                        variableName = data[2]
                        if(variableName in variables.keys()):
                            model[newProperty] = variables[variableName]
                            print "IS"
                        else:
                            print "WARNING: Variable " + variableName + " Not Found!"
                    break
                #elif(word == KEYWORDS[2]): # DEF
                    
                elif(word == KEYWORDS[3]): # {
                    if(len(data)>=2):
                        if("DEF" in data):
                            defPos = data.index("DEF")
                            name = data[defPos+1]
                            print "DEF"
                        else:
                            name = data[0]
                        model[name] = dict()
                        print "Enter Object: "  + name + " {"
                        ReadModelData(file, model[name], copy.copy(variables))
                        break
                elif(word == KEYWORDS[4]): # }
                    print line
                    return model
                    break
                #elif(word == KEYWORDS[5]): # point
                    #print line
                #elif(word == KEYWORDS[6]): # coordIndex
                    #print line
    print "Exit - End of Function"
    return model

def main(argv):
    try:                                
        opts, args = getopt.getopt(argv, "hi:o:", ["--help","input=", "output="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)                     
    
    inputFileName = None
    outputFileName = None
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
        elif opt in ("-i", "--input"):
            inputFileName = arg
        elif opt in ("-o", "--output"):
            outputFileName = arg

    if(inputFileName and outputFileName):
        file=open(inputFileName,'r')
        model = dict()
        variabes = dict()
        model = ReadModelData(file, model, variabes)
        print model
        file.close()

if __name__ == "__main__":
    main(sys.argv[1:])

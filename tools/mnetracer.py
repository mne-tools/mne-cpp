import re
import sys

num_spaces_tab = 4
tracer_macro = "MNE_TRACE()"

def parseInputArguments(argsToParse, **opts):
    mainMode = False
    caseSensitive = True
    relaxedMode = False
    inputOptions = ()
    for key, value in opts.items():
        if key == 'case_sensitive':
            caseSensitive = value
        if key == 'admit_unknown_options':
            relaxedMode = value
        if key == 'opts':
            inputOptions = value
        if key == "main_mode":
            mainMode = value
    options = {}
    for opt in inputOptions:
        if caseSensitive:
            key = opt[0]
        else:
            key = opt[0].lower()
        options[key] = opt[1]
    if mainMode:
        argsToParse2 = {}
        for arg in argsToParse[1:]:
            opt_ = arg.split("=")
            argsToParse2[opt_[0]] = opt_[1]
        argsToParse = argsToParse2        
    for arg in argsToParse:
        if caseSensitive:
            arg_adapted = arg
        else:
            arg_adapted = arg.lower()
        if arg_adapted not in options:
            if not relaxedMode:
                raise NameError('Unkown option specified.')
        else:
            options[arg_adapted] = argsToParse[arg]

    outputOptions = ()
    for i in range(len(inputOptions)):
        outputOptions += (options[inputOptions[i][0]], )
    return outputOptions

def noneIfEmpty(s):
    return "" if s is None else s

def add_tracer_macro(in_text):
    # STUDY REGEX https://regex101.com/r/Qa7BOx/1/ -or- https://regex101.com/r/NDUE03/1
    tracer_macro_line = "\n" + " " * num_spaces_tab + tracer_macro
    regExp = "(?P<methodDef>([^;\n ]+\s?)?\w+::~?\w+\([^{;]*\)[^;\n]*\n([^{]*\n)*{)(?!" + tracer_macro_line.replace("(","\(").replace(")","\)") + ")"
    match = re.search(regExp, in_text)
    if match:
        methodDef = noneIfEmpty(match.group("methodDef"))
        outText = in_text[:match.start(0)] + methodDef + tracer_macro_line + in_text[match.end(0):]
        return add_tracer_macro(outText)
    else:
        return in_text

def delete_tracer_macro(in_text):
    #STUDY https://regex101.com/r/pzKnIC/1
    tracer_macro_line = "\n" + " " * num_spaces_tab + tracer_macro
    regExp = "(?P<trace>" + tracer_macro_line.replace("(","\(").replace(")","\)") + ")"
    match = re.search(regExp, in_text)
    if match:
        methodDef = noneIfEmpty(match.group("trace"))
        outText = in_text[:match.start(0)] + in_text[match.end(0):]
        return delete_tracer_macro(outText)
    else:
        return in_text

def delete_all_tracer_macro(in_text):
    #STUDY https://regex101.com/r/pzKnIC/1
    tracer_macro_line = tracer_macro
    regExp = "(?P<trace>" + "([ \t]*)" + tracer_macro_line.replace("(","\(").replace(")","\)") + "\n?" + ")"
    match = re.search(regExp, in_text)
    if match:
        methodDef = noneIfEmpty(match.group("trace"))
        outText = in_text[:match.start(0)] + in_text[match.end(0):]
        return delete_all_tracer_macro(outText)
    else:
        return in_text        

if __name__ == "__main__":

    opts = (("file", ""),("verboseMode", False),("mode","add"))
    (input_file_name, verboseMode, mode) = parseInputArguments(sys.argv, opts = opts, admit_unknown_options=True, main_mode=True)

    if input_file_name == "":
        print("You need to specify an input file")
    else:
        if mode == "add":
            with open(input_file_name, 'r') as in_file:
                out_text = add_tracer_macro(in_file.read())
            with open(input_file_name, 'w') as out_file:
                out_file.write(out_text)
        if mode == "delete":
            with open(input_file_name, 'r') as in_file:
                out_text = delete_tracer_macro(in_file.read())
            with open(input_file_name, 'w') as out_file:
                out_file.write(out_text)
        if mode == "deleteAll":
            with open(input_file_name, 'r') as in_file:
                out_text = delete_all_tracer_macro(in_file.read())
            with open(input_file_name, 'w') as out_file:
                out_file.write(out_text)


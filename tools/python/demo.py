import mne_cpp.core
import mne_cpp.pdf_doc

mne_cpp.core.version()

projectFolder = mne_cpp.core.baseFolder()

print('The path for this MNE-CPP Project installation is ' + projectFolder)

inFile = open('c:/projects/mne-cpp/doc/gh-pages/overview.md','r', encoding='utf8')
fileText = inFile.read()
inFile.close()

aa = mne_cpp.core.extractFilePaths(fileText)

# outFile = open('file.txt', encoding='utf8 ')
# outFile.write(aa)
# outFile.close()

# print(aa)

a = 3


# dd = {}
# dd['alla'] = 4
# dd['wqrt'] = [1, 4, 55]
# dd['thtf'] = 5
# dd['eqwerqw'] = 34
# dd['eree'] = 'alajks'

# print(dd)

# def parseInputArguments(argsToParse, **opts):
#     caseSensitive = True
#     relaxedMode = False
#     inputOptions = ()
#     for key, value in opts.items():
#         if key == 'case_sensitive':
#             caseSensitive = value
#         if key == 'admit_unknown_options':
#             relaxedMode = value
#         if key == 'opts':
#             inputOptions = value
#     options = {}
#     for opt in inputOptions:
#         if caseSensitive:
#             key = opt[0]
#         else:
#             key = opt[0].lower()
#         options[key] = opt[1]
#     for arg in argsToParse:
#         if caseSensitive:
#             arg_adapted = arg
#         else:
#             arg_adapted = arg.lower()
#         if arg_adapted not in options:
#             if not relaxedMode:
#                 raise NameError('Unkown option specified.')
#         else:
#             options[arg_adapted] = argsToParse[arg]
#     return (v for k, v in options.items())


# def importantFcn(a,**inputArgs):
#     opts = (('optionA', 33), 
#             ('optionB', 'lalala'),
#             ('optionC', [range(5)]))
#     (optionA, optionB, optionC) = parseInputArguments(inputArgs, opts = opts, admit_unknown_options = True, case_sensitive = True)
#     print(a)
#     print('OptionA : ' + str(optionA))
#     print('OptionB : ' + optionB)
#     print('OptionC : ' + str(optionC))

# importantFcn('5134',optionC = 'obladiriolaride', tetetetet = '321', opTIona = 37)

# add wrapfigure to latex preamble


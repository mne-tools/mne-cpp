import re

# fp = open("index.md","r",encoding="utf8")


# text = fp.read()

# header = re.search("---(.*\n)*---",text)

# htmlText = re.search('<html>(.*\n)*</html>', text)

# p = re.compile('<html>(.*\n)*</html>')
# text2 = p.sub('',text)

juangpc@gmail.com

param\[\w+,?\w*\]\s+(\w+)\s+
<\s*img\s+src="([A-Za-z0-9/]*)" (?=width)

# print(fp)

# textRe = "[^]]*"
# imgRe = "\/.*?\.[\w:]+"
# markupRegex = '!\[({0})]\(\s*({1})\s*\)'.format(textRe, imgRe)
# imgList = re.findall(markupRegex, str)
aabbb
aaaaaa
# #####################################################

text_to_search = '''
abcdefghijklmnopqurtuvwxyz
ABCDEFGHIJKLMNOPQRSTUVWXYZ
1234567890
Ha HaHa
MetaCharacters (Need to be escaped):
. ^ $ * + ? { } [ ] \ | ( )
coreyms.com
321-555-4321
123.555.1234
123*555*1234
800-555-1234
900-555-1234
Mr. Schafer
Mr Smith
Ms Davis
Mrs. Robinson
Mr. T
'''

sentence = "Start a sentence and then bring it to an end"

pattern = re.compile(r'abc')

matches = pattern.finditer(text_to_search)

for match in matches:
    print(match)





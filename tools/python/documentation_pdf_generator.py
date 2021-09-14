import mne_cpp.core
import mne_cpp.pdf_doc as mnepdf

projectFolder = mne_cpp.core.baseFolder()
webBaseFolder = projectFolder + 'doc/gh-pages'

# webDocuments = mnepdf.scanFolder(webBaseFolder)
# print(webDocuments)

# web = mnepdf.buildWebStructure(webDocuments)
# print('Printing Web Structure:')
# print(web)

# (pathLabel, filePath, fileName, fileExt, fullPath) = mne_cpp.core.extractFilePaths('../../doc/gh-pages/pages/documentation/anonymize.md')
# (pathLabel, filePath, fileName, fileExt, fullPath) = mne_cpp.core.extractFilePaths('../../doc/gh-pages/pages/contact.md')

# inFile = open(fullPath, mode = 'r', encoding = 'utf8')
# inText = inFile.read()
# inFile.close()

# outText = mnepdf.parseUnorderedList(inText)
# outFile = open(pathLabel + filePath + fileName + '.PROCESSED' + '.' + fileExt, mode = 'w', encoding = 'utf8')
# outFile.write(outText)
# outFile.close()

inText = r'''
---
layout: default
title: Markdown kitchen sink
nav_order: 99
---
(\n(( *[-*] *)|(\s*\d+\.\s*))[^\-*\n ].+)+
(\n(( *[-*] *)|( *\d+\. *))[^\-*\n ].+)+
(\n(( *[-*] *)|( *\d+\. *))[^\-*\n ].+)+
Text can be **bold**, _italic_, or ~~strikethrough~~.

[Link to another page](another-page).

There should be whitespace between paragraphs.

There should be whitespace between paragraphs. We recommend including a README, or a file with information about your project.

# [](#header-1)Header 1

This is a normal paragraph following a header. GitHub is a code hosting platform for version control and collaboration. It lets you and others work together on projects from anywhere.

## [](#header-2)Header 2

> This is a blockquote following a header.
>
> When something is important enough, you do it even if the odds are not in your favor.

### [](#header-3)Header 3

```js
// Javascript code with syntax highlighting.
var fun = function lang(l) {
  dateformat.i18n = require('./lang/' + l)
  return true;
}
```

```ruby
# Ruby code with syntax highlighting
GitHubPages::Dependencies.gems.each do |gem, version|
  s.add_dependency(gem, "= #{version}")
end
```
- level 1 item
  - level 2 item
  - level 2 item
    - level 3 item
    - level 3 item
- level 1 item
  - level 2 item
  - level 2 item
  - level 2 item
- level 1 item
  - level 2 item
  - level 2 item
- level 1 item
'''

outText = mnepdf.parseLists(inText)





a = 3




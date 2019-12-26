# MNE-CPP Webpage

This webpage uses [Jekyll](https://jekyllrb.com/), a static website generator.

This webpage uses [just-the-docs](https://github.com/pmarsceill/just-the-docs) as a Jekyll theme.

Reading the documentation for both of these is recommended.

### How the website is structured

Github pages allows for the use of remote themes, meaning none of the theme-specific html and css are hosted here.

Pages are inside the `pages` folder. Each is a markdown file, allowing for the use of [markdown syntax](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) for creating the pages. These files get converted to html behind the scenes, so embedding html in the `.md` files works.

Images are stored in the `images` folder.

Documents, such as `.pdf` files, are stored in the `docs` folder.

Site-wide configuration is done in `_config.yml`. This is where we [configure jekyll](https://jekyllrb.com/docs/configuration/) and the theme specific options.

### Contributing

Changes are not made to this branch (gh-pages) directly. Instead, fork from the [main repository](https://github.com/mne-tools/mne-cpp), make your changes to `doc/gh-pages`, and create a pull request. Once merged, those changes will then be reflected in the gh-pages branch.

#### Editing

To edit a page, open the corresponding `.md` file and make changes. Refer to the [markdown cheatsheet](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) for how to format your page.

#### Adding

**To add a page**, create a new `.md` file for that page in the `pages` folder with a unique, relevant name. Make sure to include a header with the jekyll configuration for the page. The most important configurations are shown bellow:

```
---
title: <webpage title - will be what displays in the navigation column>
nav_order: <position in the navigation column>
parent: <what page is its parent in the navigation column>
has_children: <true/false - set to true if another page has this page as its parent>
nav_exclude: <true/false - whether to exclude this page from the navigation column>
---
```

These headers are what determine the navigation column on the left side of the page. Make sure to configure your page's navigation by ensuring your new page has the appropriate parent or child/children. A simple one, like for the landing page, might look like this :

```
---
title: Home
nav_order: 1
---
```

whereas a more complicated one, with parents and children, might look like this:

```
---
title: Contribute
parent: Community
nav_order: 1
has_children: true
---
```

For more in depth explanation of the navigation, view the [navigation documentation](https://pmarsceill.github.io/just-the-docs/docs/navigation-structure/) for the theme.

**To add an image**, add the image to the `images` folder, and link to it using [markdown syntax](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet): `![hover text](address/to/image.png)` in your page's `.md` file. When linking these, keep in mind that the file names, and file extensions, are case sensitive. The link above would not work on an images named `Image.png` or `image.PNG`

**To add a document**, such as a pdf, and embed it in the page, add the document to the `docs` file, and use the following snippet of html in your page's `.md` file,

```
<embed src="https://mne-tools.github.io/mne-cpp/docs/file-name.pdf" width="800px" height="500px" type="application/pdf" />
```

, replacing `file-name.pdf` with the name of your document.

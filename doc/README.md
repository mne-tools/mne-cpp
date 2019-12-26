# MNE-CPP Webpage

This webpage uses [just-the-docs](https://github.com/pmarsceill/just-the-docs) as a theme.

This webpage uses [Jekyll](https://jekyllrb.com/), a static website generator.

Reading the documentation for both of these is recommended.

### How the website is structured

Github pages allows to the use of remote themes, meaning none of the theme-specific html and css are hosted here.

Pages are inside the `pages` folder. Each is a markdown file, allowing for the use of [markdown syntax](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) for creating the pages. These files get converted to html behind the scenes, so embedding html in the `.md` files works.

Images are stored in the `images` folder. When linking these, keep in mind that the file names, and file extensions, are case sensitive.

Documents, such as `.pdf` files, are stored in the `docs` folder.

Site-wide configuration is done in `_config.yml`. This is where we configure jekyll and the theme specific options.

### Contributing

Changes are not made to this branch (gh-pages) directly. Instead, fork from the [main repository](https://github.com/mne-tools/mne-cpp), make your changes to the `doc` folder, and create a pull request. Once merged, those changes will then be reflected here.

#### Editing

To edit a page, open the corresponding `.md` file and make changes. Refer to the [markdown cheatsheet](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) for how to format your page.

#### Adding

To add a page, create a new `.md` file for that page in the `pages` folder with a unique, relevant name. Make sure to include a header with the jekyll configuration for the page:

```
---
title: <webpage title - will be what displays in the navigation column>
nav_order: <position in the navigation column>
parent: <what page is its parent in the navigation column>
has_children: <true/false - set to true if another page has this page as its parent>
nav_exclude: <true/false - whether to exclude this page from the navigation column>
---
```

These headers are what determine the navigation column on the left side of the page. Make sure to configure your page's navigation by ensuring your new page has the appropriate parent or child/children. For more in depth explanation of the navigation, view the [navigation documentation](https://pmarsceill.github.io/just-the-docs/docs/navigation-structure/) for the theme.

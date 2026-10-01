import { themes as prismThemes } from 'prism-react-renderer';
import type { Config } from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';
import remarkMath from 'remark-math';
import rehypeKatex from 'rehype-katex';

// Determine if this is a dev or stable build (set via CI env var)
const siteEnv = process.env.MNECPP_SITE_ENV || 'stable';
const isDev = siteEnv === 'dev';
// Latest release; tools/quality/tests/test_website_config.py keeps it equal to src/CMakeLists.txt.
export const stableVersion = '2.3.0';
const versionLabel = isDev ? 'dev (latest)' : `v${stableVersion}`;

const config: Config = {
    title: 'MNE-CPP',
    tagline: 'The C++ framework for real-time functional brain imaging.',
    favicon: 'img/favicon.ico',

    url: 'https://mne-cpp.github.io',
    // Allow CI to override baseUrl for dev builds (deployed under /dev/)
    baseUrl: process.env.DOCUSAURUS_BASE_URL || '/',

    organizationName: 'mne-cpp',
    projectName: 'mne-cpp.github.io',

    onBrokenLinks: 'throw',
    onBrokenAnchors: 'throw',

    customFields: {
        version: stableVersion,
    },

    i18n: {
        defaultLocale: 'en',
        locales: ['en'],
    },

    presets: [
        [
            'classic',
            {
                docs: {
                    sidebarPath: './sidebars.ts',
                    remarkPlugins: [remarkMath],
                    rehypePlugins: [rehypeKatex],
                },
                blog: false,
                theme: {
                    customCss: './src/css/custom.css',
                },
            } satisfies Preset.Options,
        ],
    ],

    themes: [
        '@docusaurus/theme-mermaid',
        [
            '@easyops-cn/docusaurus-search-local',
            {
                hashed: true,
                indexDocs: true,
                indexBlog: false,
                docsRouteBasePath: '/docs',
                searchBarShortcutHint: true,
                searchBarPosition: 'right',
            },
        ],
    ],

    markdown: {
        format: 'mdx',
        mermaid: true,
        hooks: {
            onBrokenMarkdownLinks: 'throw',
            // Only the generated manual screenshots (/img/manual/auto/, built by
            // mne_doc_shots, TASK 5) may be absent; any other missing image fails.
            onBrokenMarkdownImages: ({url, sourceFilePath}) => {
                if (url.startsWith('/img/manual/auto/')) {
                    console.warn(`[WARNING] generated screenshot ${url} not built (${sourceFilePath})`);
                    return;
                }
                throw new Error(`Markdown image ${url} not found (${sourceFilePath})`);
            },
        },
    },

    stylesheets: [
        '/katex.min.css',
    ],

    themeConfig: {
        image: 'img/mne-cpp-social-card.png',
        colorMode: {
            defaultMode: 'dark',
            disableSwitch: false,
            respectPrefersColorScheme: true,
        },
        navbar: {
            title: '',
            logo: {
                alt: 'MNE-CPP Logo',
                src: 'img/logo.svg',
                srcDark: 'img/logo-dark.svg',
            },
            items: [
                {
                    type: 'docSidebar',
                    sidebarId: 'docsSidebar',
                    position: 'left',
                    label: 'Documentation',
                },
                {
                    type: 'docSidebar',
                    sidebarId: 'developmentSidebar',
                    position: 'left',
                    label: 'Development',
                },
                {
                    type: 'docSidebar',
                    sidebarId: 'apiSidebar',
                    position: 'left',
                    label: 'API',
                },
                {
                    to: '/download',
                    label: 'Download',
                    position: 'left',
                },
                {
                    type: 'dropdown',
                    label: versionLabel,
                    position: 'right',
                    className: 'navbar-version-dropdown',
                    items: [
                        {
                            label: `v${stableVersion} (Stable)`,
                            href: 'https://mne-cpp.github.io/',
                        },
                        {
                            label: 'dev (Latest Development)',
                            href: 'https://mne-cpp.github.io/dev/',
                        },
                    ],
                },
                {
                    href: 'https://github.com/mne-tools/mne-cpp',
                    position: 'right',
                    className: 'header-github-link',
                    'aria-label': 'GitHub repository',
                },
            ],
        },
        footer: {
            style: 'dark',
            links: [
                {
                    title: 'Documentation',
                    items: [
                        { label: 'Getting Started', to: '/docs/overview' },
                        { label: 'Manual', to: '/docs/manual/intro' },
                        { label: 'Publications', to: '/docs/publications' },
                    ],
                },
                {
                    title: 'Development',
                    items: [
                        { label: 'Build from Source', to: '/docs/development/buildguide-cmake' },
                        { label: 'Contribute', to: '/docs/development/contribute' },
                        { label: 'API Reference', to: '/docs/api' },
                    ],
                },
                {
                    title: 'Community',
                    items: [
                        { label: 'GitHub', href: 'https://github.com/mne-tools/mne-cpp' },
                        { label: 'How to Cite', to: '/docs/cite' },
                        { label: 'Legal Notice', to: '/docs/overview#legal-notice' },
                    ],
                },
            ],
            copyright: `Copyright © ${new Date().getFullYear()} MNE-CPP. Built with Docusaurus.`,
        },
        prism: {
            theme: prismThemes.github,
            darkTheme: prismThemes.dracula,
            additionalLanguages: ['cpp', 'cmake', 'bash'],
        },
    } satisfies Preset.ThemeConfig,
};

export default config;

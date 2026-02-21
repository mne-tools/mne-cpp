import { themes as prismThemes } from 'prism-react-renderer';
import type { Config } from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

const config: Config = {
    title: 'MNE-CPP',
    tagline: 'The C++ framework for real-time functional brain imaging.',
    favicon: 'img/favicon.ico',

    url: 'https://mne-cpp.github.io',
    baseUrl: '/',

    organizationName: 'mne-cpp',
    projectName: 'mne-cpp.github.io',

    onBrokenLinks: 'warn',
    onBrokenMarkdownLinks: 'warn',

    customFields: {
        version: '2.0.0',
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
                    editUrl: 'https://github.com/mne-tools/mne-cpp/tree/main/doc/website/',
                },
                blog: false,
                theme: {
                    customCss: './src/css/custom.css',
                },
            } satisfies Preset.Options,
        ],
    ],

    markdown: {
        format: 'md',
    },

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
                    href: 'https://mne-cpp.github.io/doxygen-api/',
                    label: 'API Reference',
                    position: 'left',
                },
                {
                    to: '/download',
                    label: 'Download',
                    position: 'left',
                },
                {
                    type: 'html',
                    position: 'right',
                    value: '<span class="navbar-version-badge">v2.0.0</span>',
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
                        { label: 'MNE Scan', to: '/docs/documentation/scan' },
                        { label: 'MNE Analyze', to: '/docs/documentation/analyze' },
                    ],
                },
                {
                    title: 'Development',
                    items: [
                        { label: 'Build from Source', to: '/docs/development/buildguide' },
                        { label: 'Contribute', to: '/docs/development/contribute' },
                        { label: 'API Reference', href: 'https://mne-cpp.github.io/doxygen-api/' },
                    ],
                },
                {
                    title: 'Community',
                    items: [
                        { label: 'GitHub', href: 'https://github.com/mne-tools/mne-cpp' },
                        { label: 'Contact', to: '/docs/contact' },
                        { label: 'How to Cite', to: '/docs/cite' },
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

import type { SidebarsConfig } from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
    docsSidebar: [
        'overview',
        {
            type: 'category',
            label: 'Manual',
            link: { type: 'doc', id: 'manual/intro' },
            items: [
                {
                    type: 'category',
                    label: 'Background',
                    items: [
                        'manual/workflow',
                        'manual/forward',
                        'manual/inverse',
                        'manual/ssp',
                    ],
                },
                {
                    type: 'category',
                    label: 'Applications',
                    items: [
                        {
                            type: 'category',
                            label: 'MNE Scan',
                            link: { type: 'doc', id: 'documentation/scan' },
                            items: [
                                'documentation/scan-prerecordeddata',
                                'documentation/scan-sourceloc',
                                'documentation/scan-headmonitoring',
                                'documentation/scan-forward',
                                'documentation/scan-brainamp',
                                'documentation/scan-gusbamp',
                                'documentation/scan-eegosports',
                                'documentation/scan-natus',
                                'documentation/scan-ftbuffer',
                                'documentation/scan-lsl',
                                'documentation/scan-tmsi',
                                'documentation/scan-interfacewithanalyze',
                            ],
                        },
                        {
                            type: 'category',
                            label: 'MNE Analyze',
                            link: { type: 'doc', id: 'documentation/analyze' },
                            items: [
                                'documentation/analyze-dataloader',
                                'documentation/analyze-datamanager',
                                'documentation/analyze-rawdataviewer',
                                'documentation/analyze-annotationmanager',
                                'documentation/analyze-average',
                                'documentation/analyze-filter',
                                'documentation/analyze-channelselect',
                                'documentation/analyze-coregistration',
                                'documentation/analyze-scaling',
                                'documentation/analyze-dipolefit',
                                'documentation/analyze-realtime',
                            ],
                        },
                        'documentation/anonymize',
                        'documentation/inspect',
                        'manual/browse-raw',
                        'manual/analyze-manual',
                    ],
                },
                {
                    type: 'category',
                    label: 'Command-Line Tools',
                    link: { type: 'doc', id: 'manual/tools-overview' },
                    items: [
                        'manual/tools-watershed-bem',
                        'manual/tools-flash-bem',
                        'manual/tools-surf2bem',
                        'manual/tools-setup-forward-model',
                        'manual/tools-setup-mri',
                        'manual/tools-forward-solution',
                        'manual/tools-compute-raw-inverse',
                        'manual/tools-dipole-fit',
                        'manual/tools-edf2fiff',
                        'manual/tools-show-fiff',
                        'manual/tools-rt-server',
                    ],
                },
            ],
        },
        'publications',
        'cite',
    ],
    developmentSidebar: [
        {
            type: 'category',
            label: 'Build & Deploy',
            items: [
                'development/buildguide-cmake',
                {
                    type: 'category',
                    label: 'WebAssembly',
                    link: { type: 'doc', id: 'development/wasm' },
                    items: [
                        'development/wasm-buildguide',
                        'development/wasm-testing',
                    ],
                },
                {
                    type: 'category',
                    label: 'Continuous Integration',
                    link: { type: 'doc', id: 'development/ci' },
                    items: [
                        'development/ci-ghactions',
                        'development/ci-deployment',
                        'development/ci-releasecycle',
                    ],
                },
            ],
        },
        {
            type: 'category',
            label: 'Contributing',
            link: { type: 'doc', id: 'development/contribute' },
            items: [
                'development/contr-guide',
                'development/contr-style',
                'development/contr-git',
                'development/contr-docuimprovements',
                'development/writingtest',
                'development/contr-mnetracer',
            ],
        },
        {
            type: 'category',
            label: 'Library API',
            link: { type: 'doc', id: 'development/api' },
            items: [
                'development/api-disp3d',
                'development/api-connectivity',
            ],
        },
        {
            type: 'category',
            label: 'Plugin Development',
            items: [
                {
                    type: 'category',
                    label: 'MNE Scan',
                    link: { type: 'doc', id: 'development/scan' },
                    items: [
                        'development/scan-acq',
                        'development/scan-plugin',
                    ],
                },
                {
                    type: 'category',
                    label: 'MNE Analyze',
                    link: { type: 'doc', id: 'development/analyze' },
                    items: [
                        'development/analyze-plugin',
                        'development/analyze-event',
                        'development/analyze-datamodel',
                    ],
                },
            ],
        },
    ],
};

export default sidebars;

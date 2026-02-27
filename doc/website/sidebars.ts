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
                            link: { type: 'doc', id: 'manual/scan' },
                            items: [
                                'manual/scan-prerecordeddata',
                                'manual/scan-sourceloc',
                                'manual/scan-headmonitoring',
                                'manual/scan-forward',
                                'manual/scan-brainamp',
                                'manual/scan-gusbamp',
                                'manual/scan-eegosports',
                                'manual/scan-natus',
                                'manual/scan-ftbuffer',
                                'manual/scan-lsl',
                                'manual/scan-tmsi',
                                'manual/scan-interfacewithanalyze',
                            ],
                        },
                        {
                            type: 'category',
                            label: 'MNE Analyze',
                            link: { type: 'doc', id: 'manual/analyze' },
                            items: [
                                'manual/analyze-dataloader',
                                'manual/analyze-datamanager',
                                'manual/analyze-rawdataviewer',
                                'manual/analyze-annotationmanager',
                                'manual/analyze-average',
                                'manual/analyze-filter',
                                'manual/analyze-channelselect',
                                'manual/analyze-coregistration',
                                'manual/analyze-scaling',
                                'manual/analyze-dipolefit',
                                'manual/analyze-realtime',
                            ],
                        },
                        'manual/anonymize',
                        'manual/inspect',
                        'manual/browse-raw',
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
                        'manual/tools-make-source-space',
                        'manual/tools-inverse-operator',
                        'manual/tools-compute-mne',
                        'manual/tools-compute-raw-inverse',
                        'manual/tools-dipole-fit',
                        'manual/tools-process-raw',
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

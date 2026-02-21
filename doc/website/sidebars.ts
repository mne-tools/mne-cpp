import type { SidebarsConfig } from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
    docsSidebar: [
        'overview',
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
        'feature-catalogue',
        'publications',
        'cite',
        'contact',
        'projects',
    ],
    developmentSidebar: [
        {
            type: 'category',
            label: 'Library API',
            items: [
                'development/api',
                'development/api-disp3d',
                'development/api-connectivity',
            ],
        },
        {
            type: 'category',
            label: 'MNE Scan Development',
            items: [
                'development/scan',
                'development/scan-acq',
                'development/scan-plugin',
            ],
        },
        {
            type: 'category',
            label: 'MNE Analyze Development',
            items: [
                'development/analyze',
                'development/analyze-plugin',
                'development/analyze-event',
                'development/analyze-datamodel',
            ],
        },
        {
            type: 'category',
            label: 'WebAssembly',
            items: [
                'development/wasm',
                'development/wasm-buildguide',
                'development/wasm-testing',
            ],
        },
        {
            type: 'category',
            label: 'Contributing',
            items: [
                'development/contribute',
                'development/contr-guide',
                'development/contr-docuimprovements',
                'development/contr-style',
                'development/contr-git',
                'development/contr-mnetracer',
            ],
        },
        {
            type: 'category',
            label: 'Build from Source',
            items: [
                'development/buildguide',
                'development/buildguide-cmake',
                'development/buildguide-qmake',
            ],
        },
        {
            type: 'category',
            label: 'Continuous Integration',
            items: [
                'development/ci',
                'development/ci-ghactions',
                'development/ci-deployment',
                'development/ci-releasecycle',
            ],
        },
        'development/writingtest',
    ],
};

export default sidebars;

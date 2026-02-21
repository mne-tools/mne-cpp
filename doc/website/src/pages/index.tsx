import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import { useColorMode } from '@docusaurus/theme-common';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import styles from './index.module.css';

const applications = [
    {
        title: 'MNE Scan',
        description: 'Real-time acquisition and processing of MEG/EEG data with a modular plugin architecture.',
        link: '/docs/documentation/scan',
        icon: 'img/icon_mne_scan_256x256.png',
    },
    {
        title: 'MNE Analyze',
        description: 'Visualization, filtering, averaging, co-registration, and source localization.',
        link: '/docs/documentation/analyze',
        icon: 'img/icon_mne-analyze_256x256.png',
    },
    {
        title: 'C++ Library',
        description: 'Cross-platform API built on Qt & Eigen. Build your own neuroscience applications.',
        link: '/docs/development/api',
        icon: 'img/icon_mne-lib_256x256.png',
    },
];

const institutions = [
    { name: 'Martinos Center', url: 'https://martinos.org/', logo: 'img/institution_logos/martinos.svg' },
    { name: 'TU Ilmenau', url: 'https://www.tu-ilmenau.de/', logo: 'img/institution_logos/Ilmenau.svg' },
    { name: 'Massachusetts General Hospital', url: 'https://www.massgeneral.org/', logo: 'img/institution_logos/MGH.svg' },
    { name: "Boston Children's Hospital", url: 'http://www.childrenshospital.org/', logo: 'img/institution_logos/bch.svg' },
    { name: 'Harvard Medical School', url: 'https://hms.harvard.edu/', logo: 'img/institution_logos/harvard.svg' },
    { name: 'UTHealth Houston', url: 'https://www.uth.edu/', logo: 'img/institution_logos/uthealth.svg' },
    { name: 'Universität Magdeburg', url: 'https://www.uni-magdeburg.de/', logo: 'img/institution_logos/magdeburg.svg' },
    { name: 'Forschungscampus Stimulate', url: 'https://www.forschungscampus-stimulate.de/', logo: 'img/institution_logos/stimulate_magdeburg.svg' },
    { name: 'UMIT', url: 'https://umit.at/', logo: 'img/institution_logos/umit.svg' },
    { name: 'Universitätsklinikum Jena', url: 'https://www.uniklinikum-jena.de/', logo: 'img/institution_logos/jena.svg' },
];

const funders = [
    { name: 'NIH / NIBIB', logo: 'img/funding_logos/nibib.svg', url: 'https://www.nibib.nih.gov/' },
    { name: 'NIH', logo: 'img/funding_logos/nih.svg', url: 'https://www.nih.gov/' },
    { name: 'DFG', logo: 'img/funding_logos/dfg.svg', url: 'https://www.dfg.de/' },
    { name: 'FWF', logo: 'img/funding_logos/fwf.svg', url: 'https://www.fwf.ac.at/' },
    { name: 'AWS', logo: 'img/funding_logos/aws.svg', url: 'https://aws.amazon.com/' },
    { name: 'Azure', logo: 'img/funding_logos/azure.svg', url: 'https://azure.microsoft.com/' },
];

function WaveBackground() {
    return (
        <div className={styles.waveWrap} aria-hidden="true">
            <svg className={styles.waveSvg} viewBox="0 0 1440 500" preserveAspectRatio="none">
                {/* Semi-transparent filled bands below each wave */}
                <path className={styles.waveFill1}
                    d="M-200,260 C-100,210 0,310 100,240 C200,170 300,330 400,260 C500,190 600,350 700,260 C800,170 900,330 1000,240 C1100,150 1200,350 1300,260 C1400,170 1500,330 1600,240 C1700,150 1800,330 1900,260 L1900,500 L-200,500 Z" />
                <path className={styles.waveFill2}
                    d="M-200,220 C-80,170 40,270 160,200 C280,130 400,290 520,220 C640,150 760,310 880,220 C1000,130 1120,290 1240,200 C1360,110 1480,290 1600,220 C1720,150 1840,290 1960,200 L1960,500 L-200,500 Z" />
                <path className={styles.waveFill3}
                    d="M-200,340 C-60,290 80,390 220,330 C360,270 500,410 640,340 C780,270 920,410 1060,340 C1200,270 1340,410 1480,340 C1620,270 1760,410 1900,340 L1900,500 L-200,500 Z" />

                {/* EEG-like trace lines */}
                <path className={styles.wave1}
                    d="M-200,260 C-100,210 0,310 100,240 C200,170 300,330 400,260 C500,190 600,350 700,260 C800,170 900,330 1000,240 C1100,150 1200,350 1300,260 C1400,170 1500,330 1600,240 C1700,150 1800,330 1900,260" />
                <path className={styles.wave2}
                    d="M-200,220 C-80,170 40,270 160,200 C280,130 400,290 520,220 C640,150 760,310 880,220 C1000,130 1120,290 1240,200 C1360,110 1480,290 1600,220 C1720,150 1840,290 1960,200" />
                <path className={styles.wave3}
                    d="M-200,340 C-60,290 80,390 220,330 C360,270 500,410 640,340 C780,270 920,410 1060,340 C1200,270 1340,410 1480,340 C1620,270 1760,410 1900,340" />
                <path className={styles.wave4}
                    d="M-200,160 C-40,120 120,200 280,150 C440,100 600,220 760,160 C920,100 1080,220 1240,160 C1400,100 1560,220 1720,160" />
                <path className={styles.wave5}
                    d="M-200,410 C-80,370 60,450 200,400 C340,350 480,460 620,410 C760,360 900,460 1040,410 C1180,360 1320,460 1460,410 C1600,360 1740,450 1880,410" />
            </svg>
        </div>
    );
}

function HeroLogo() {
    const { colorMode } = useColorMode();
    const src = colorMode === 'dark' ? 'img/logo-dark.svg' : 'img/logo.svg';
    return <img src={src} alt="MNE-CPP" className={styles.heroLogo} />;
}

export default function Home(): JSX.Element {
    const { siteConfig } = useDocusaurusContext();
    const version = siteConfig.customFields?.version as string;

    return (
        <Layout
            title="Home"
            description="MNE-CPP — An open-source C++ framework for human brain mapping and neuroimaging">

            <header className={styles.hero}>
                <WaveBackground />
                <div className={styles.heroInner}>
                    <HeroLogo />
                    <p className={styles.heroTagline}>{siteConfig.tagline}</p>
                    <div className={styles.heroCta}>
                        <Link className="button button--primary button--lg" to="/docs/overview">
                            Get Started
                        </Link>
                        <Link className="button button--outline button--lg" to="/download">
                            Download
                        </Link>
                    </div>
                </div>
            </header>

            <main>
                {/* Applications */}
                <section className={styles.apps}>
                    <div className="container">
                        <div className="row">
                            {applications.map((app, idx) => (
                                <div key={idx} className="col col--4">
                                    <Link to={app.link} className={styles.appCard}>
                                        <img src={app.icon} alt={app.title} className={styles.appIcon} />
                                        <Heading as="h3" className={styles.appTitle}>{app.title}</Heading>
                                        <p className={styles.appDesc}>{app.description}</p>
                                        <span className={styles.appLink}>Learn more →</span>
                                    </Link>
                                </div>
                            ))}
                        </div>
                    </div>
                </section>

                {/* Supported by Researchers */}
                <section className={styles.logoSection}>
                    <div className="container text--center">
                        <Heading as="h2" className={styles.sectionTitle}>Supported by Researchers at</Heading>
                        <div className={styles.logoGrid}>
                            {institutions.map((inst, idx) => (
                                <a key={idx} href={inst.url} target="_blank" rel="noopener noreferrer" className={styles.logoLink}>
                                    <img src={inst.logo} alt={inst.name} title={inst.name} />
                                </a>
                            ))}
                        </div>
                    </div>
                </section>

                {/* Funding */}
                <section className={clsx(styles.logoSection, styles.logoSectionAlt)}>
                    <div className="container text--center">
                        <Heading as="h2" className={styles.sectionTitle}>Funding</Heading>
                        <div className={styles.logoGrid}>
                            {funders.map((f, idx) => (
                                <a key={idx} href={f.url} target="_blank" rel="noopener noreferrer" className={styles.logoLink}>
                                    <img src={f.logo} alt={f.name} title={f.name} />
                                </a>
                            ))}
                        </div>
                    </div>
                </section>
            </main>
        </Layout>
    );
}

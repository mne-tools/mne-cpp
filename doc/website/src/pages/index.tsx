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
        link: '/docs/manual/scan',
        icon: 'img/icon_mne_scan_256x256.png',
    },
    {
        title: 'MNE Analyze',
        description: 'Visualization, filtering, averaging, co-registration, and source localization.',
        link: '/docs/manual/analyze',
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
    { name: 'UTHealth Houston', url: 'https://www.uth.edu/', logo: 'img/institution_logos/uthealth.png' },
    { name: 'Universität Magdeburg', url: 'https://www.uni-magdeburg.de/', logo: 'img/institution_logos/magdeburg.svg' },
    { name: 'Forschungscampus Stimulate', url: 'https://www.forschungscampus-stimulate.de/', logo: 'img/institution_logos/stimulate_magdeburg.svg' },
    { name: 'Universität Innsbruck', url: 'https://www.uibk.ac.at/', logo: 'img/institution_logos/innsbruck.png' },
    { name: 'Universitätsklinikum Jena', url: 'https://www.uniklinikum-jena.de/', logo: 'img/institution_logos/jena.png' },
];

const funders = [
    { name: 'NIH / NIBIB', logo: 'img/funding_logos/nibib.svg', url: 'https://www.nibib.nih.gov/' },
    { name: 'NIH', logo: 'img/funding_logos/nih.svg', url: 'https://www.nih.gov/' },
    { name: 'DFG', logo: 'img/funding_logos/dfg.png', url: 'https://www.dfg.de/' },
    { name: 'FWF', logo: 'img/funding_logos/fwf.svg', url: 'https://www.fwf.ac.at/' },
    { name: 'AWS', logo: 'img/funding_logos/aws.svg', url: 'https://aws.amazon.com/' },
    { name: 'Azure', logo: 'img/funding_logos/azure.png', url: 'https://azure.microsoft.com/' },
];

const contributors = [
    { login: 'LorenzE', contributions: 3902 },
    { login: 'chdinh', contributions: 2868 },
    { login: 'gabrielbmotta', contributions: 1326 },
    { login: 'juangpc', contributions: 1207 },
    { login: 'RDoerfel', contributions: 626 },
    { login: '1DIce', contributions: 267 },
    { login: 'LostSign', contributions: 194 },
    { login: 'ViktorKL', contributions: 179 },
    { login: 'GBeret', contributions: 169 },
    { login: 'rickytjen', contributions: 156 },
    { login: 'JanaKiesel', contributions: 144 },
    { login: 'floschl', contributions: 76 },
    { login: 'joewalter', contributions: 64 },
    { login: 'sheinke', contributions: 58 },
    { login: 'Andrey1994', contributions: 46 },
    { login: 'TiKunze', contributions: 41 },
    { login: 'farndt', contributions: 39 },
    { login: 'imsorryk', contributions: 35 },
    { login: 'louiseichhorst', contributions: 20 },
    { login: 'femigr', contributions: 17 },
    { login: 'johaenns', contributions: 14 },
    { login: 'jobehrens', contributions: 12 },
    { login: 'cdoshi', contributions: 12 },
    { login: 'cpieloth', contributions: 11 },
    { login: 'alexrockhill', contributions: 10 },
    { login: 'buildqa', contributions: 9 },
    { login: 'SachdevaS', contributions: 7 },
    { login: 'er06645810', contributions: 6 },
    { login: 'mfarisyahya', contributions: 6 },
    { login: 'ag-fieldline', contributions: 5 },
    { login: 'betaha', contributions: 4 },
    { login: 'Julius-L', contributions: 4 },
    { login: 'MKlamke', contributions: 4 },
    { login: 'fjpolo', contributions: 4 },
    { login: 'benkay86', contributions: 3 },
    { login: 'jasmainak', contributions: 3 },
    { login: 'PetrosSimidyan', contributions: 3 },
    { login: 'larsoner', contributions: 2 },
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

                {/* Contributors */}
                <section className={styles.contributorsSection}>
                    <div className="container text--center">
                        <Heading as="h2" className={styles.sectionTitle}>Contributors</Heading>
                        <p className={styles.contributorsSubtitle}>
                            MNE-CPP is built by an open-source community. Thank you to all our contributors!
                        </p>
                        <div className={styles.contributorsGrid}>
                            {contributors.map((c, idx) => (
                                <a
                                    key={idx}
                                    href={`https://github.com/${c.login}`}
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    className={styles.contributorLink}
                                    title={`${c.login} — ${c.contributions} commits`}
                                >
                                    <img
                                        src={`https://avatars.githubusercontent.com/${c.login}?s=96`}
                                        alt={c.login}
                                        className={styles.contributorAvatar}
                                        loading="lazy"
                                    />
                                    <span className={styles.contributorName}>{c.login}</span>
                                </a>
                            ))}
                        </div>
                    </div>
                </section>

                {/* Supported by Researchers */}
                <section className={styles.logoSection}>
                    <div className="container text--center">
                        <Heading as="h2" className={styles.sectionTitle}>Research Partners</Heading>
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

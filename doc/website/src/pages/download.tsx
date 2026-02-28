import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';
import React, { useEffect, useRef, useState } from 'react';

/* ────────────────────────────────────────────────────────────────
 *  Types
 * ──────────────────────────────────────────────────────────────── */

interface GitHubAsset {
    name: string;
    browser_download_url: string;
    size: number;
    updated_at: string;
}

interface GitHubRelease {
    tag_name: string;
    name: string;
    published_at: string;
    html_url: string;
    target_commitish: string;
    assets: GitHubAsset[];
}

/* ────────────────────────────────────────────────────────────────
 *  Helpers
 * ──────────────────────────────────────────────────────────────── */

const GH_API = 'https://api.github.com/repos/mne-tools/mne-cpp/releases';

function fmtSize(bytes: number): string {
    if (bytes >= 1e9) return (bytes / 1e9).toFixed(1) + ' GB';
    if (bytes >= 1e6) return (bytes / 1e6).toFixed(1) + ' MB';
    if (bytes >= 1e3) return (bytes / 1e3).toFixed(1) + ' KB';
    return bytes + ' B';
}

function fmtDate(iso: string): string {
    return new Date(iso).toLocaleDateString('en-US', {
        year: 'numeric', month: 'short', day: 'numeric',
    });
}

function shortSha(sha: string): string {
    return sha.slice(0, 7);
}

/** Returns true when the string looks like a full or abbreviated commit SHA. */
function isCommitSha(s: string): boolean {
    return /^[0-9a-f]{7,40}$/i.test(s);
}

/* ────────────────────────────────────────────────────────────────
 *  Platform-specific asset matching
 * ──────────────────────────────────────────────────────────────── */

interface PlatformDef {
    label: string;
    match: (name: string) => boolean;
}

const INSTALLER_PLATFORMS: PlatformDef[] = [
    { label: 'Windows (.exe)', match: (n) => n.endsWith('.exe') },
    { label: 'macOS (.dmg)', match: (n) => n.endsWith('.dmg') },
    { label: 'Linux (.run)', match: (n) => n.endsWith('.run') },
];

const DEV_DYNAMIC: PlatformDef[] = [
    { label: 'Windows (x86_64)', match: (n) => n.includes('windows-dynamic') },
    { label: 'Linux (x86_64)', match: (n) => n.includes('linux-dynamic') },
    { label: 'macOS (ARM64)', match: (n) => n.includes('macos-dynamic') },
];

const DEV_STATIC: PlatformDef[] = [
    { label: 'Windows (x86_64)', match: (n) => n.includes('windows-static') },
    { label: 'Linux (x86_64)', match: (n) => n.includes('linux-static') },
    { label: 'macOS (ARM64)', match: (n) => n.includes('macos-static') },
];

const DEV_WASM: PlatformDef[] = [
    { label: 'WebAssembly', match: (n) => n.includes('wasm') },
];

const STABLE_DYNAMIC: PlatformDef[] = [
    { label: 'Windows (x86_64)', match: (n) => n.includes('windows-dynamic') },
    { label: 'Linux (x86_64)', match: (n) => n.includes('linux-dynamic') },
    { label: 'macOS (x86_64)', match: (n) => n.includes('macos-dynamic') },
];

const STABLE_STATIC: PlatformDef[] = [
    { label: 'Windows (x86_64)', match: (n) => n.includes('windows-static') },
    { label: 'Linux (x86_64)', match: (n) => n.includes('linux-static') },
    { label: 'macOS (x86_64)', match: (n) => n.includes('macos-static') },
];

/* ────────────────────────────────────────────────────────────────
 *  Components
 * ──────────────────────────────────────────────────────────────── */

function AssetTable({ assets, platforms }: { assets: GitHubAsset[]; platforms: PlatformDef[] }) {
    const rows = platforms
        .map((p) => ({ platform: p, asset: assets.find((a) => p.match(a.name)) }))
        .filter((r) => r.asset);

    if (rows.length === 0) return null;

    return (
        <table className="dl-table">
            <thead>
                <tr>
                    <th>Platform</th>
                    <th>File</th>
                    <th className="dl-table__right">Size</th>
                </tr>
            </thead>
            <tbody>
                {rows.map((r, i) => (
                    <tr key={i}>
                        <td>{r.platform.label}</td>
                        <td>
                            <a href={r.asset!.browser_download_url} rel="noopener noreferrer">
                                {r.asset!.name}
                            </a>
                        </td>
                        <td className="dl-table__right dl-table__mono">{fmtSize(r.asset!.size)}</td>
                    </tr>
                ))}
            </tbody>
        </table>
    );
}

/** Compact asset list for side-by-side layout */
function AssetList({ assets, platforms, heading }: { assets: GitHubAsset[]; platforms: PlatformDef[]; heading?: string }) {
    const rows = platforms
        .map((p) => ({ platform: p, asset: assets.find((a) => p.match(a.name)) }))
        .filter((r) => r.asset);

    if (rows.length === 0) return null;

    return (
        <div className="dl-asset-group">
            {heading && <div className="dl-asset-group__heading">{heading}</div>}
            <ul className="dl-asset-list">
                {rows.map((r, i) => (
                    <li key={i}>
                        <a href={r.asset!.browser_download_url} rel="noopener noreferrer">
                            {r.platform.label}
                        </a>
                        <span className="dl-asset-list__size">{fmtSize(r.asset!.size)}</span>
                    </li>
                ))}
            </ul>
        </div>
    );
}

function ReleaseMeta({ release, commitSha }: { release: GitHubRelease; commitSha?: string }) {
    const raw = commitSha || release.target_commitish;
    // Normalise legacy branch name for display
    const ref = raw === 'master' ? 'main' : raw;
    const isSha = ref ? isCommitSha(ref) : false;
    const refUrl = ref
        ? isSha
            ? `https://github.com/mne-tools/mne-cpp/commit/${ref}`
            : `https://github.com/mne-tools/mne-cpp/tree/${ref}`
        : undefined;
    const refLabel = ref
        ? isSha ? shortSha(ref) : ref
        : undefined;

    return (
        <div className="dl-meta">
            <span className="dl-meta__tag">{release.tag_name}</span>
            <span className="dl-meta__sep" aria-hidden="true">·</span>
            <span className="dl-meta__date">{fmtDate(release.published_at)}</span>
            {refUrl && refLabel && (
                <>
                    <span className="dl-meta__sep" aria-hidden="true">·</span>
                    <a
                        href={refUrl}
                        className="dl-meta__sha"
                        target="_blank"
                        rel="noopener noreferrer"
                    >
                        {refLabel}
                    </a>
                </>
            )}
        </div>
    );
}

function Skeleton() {
    return (
        <div className="dl-skeleton">
            <div className="dl-skeleton__bar" style={{ width: '55%' }} />
            <div className="dl-skeleton__bar" style={{ width: '75%' }} />
            <div className="dl-skeleton__bar" style={{ width: '40%' }} />
        </div>
    );
}

/* ────────────────────────────────────────────────────────────────
 *  Table of Contents (right sidebar)
 * ──────────────────────────────────────────────────────────────── */

const TOC_ITEMS: { id: string; label: string }[] = [
    { id: 'releases', label: 'Releases' },
    { id: 'browser-apps', label: 'Browser-Based Apps' },
    { id: 'sample-data', label: 'Sample Data' },
    { id: 'quick-start', label: 'Quick Start' },
    { id: 'verifying', label: 'Verifying Downloads' },
    { id: 'requirements', label: 'System Requirements' },
];

function PageToc() {
    const [active, setActive] = useState<string>('');

    useEffect(() => {
        const ids = TOC_ITEMS.map((t) => t.id);
        const observer = new IntersectionObserver(
            (entries) => {
                for (const e of entries) {
                    if (e.isIntersecting) {
                        setActive(e.target.id);
                        break;
                    }
                }
            },
            { rootMargin: '-80px 0px -60% 0px', threshold: 0 },
        );
        ids.forEach((id) => {
            const el = document.getElementById(id);
            if (el) observer.observe(el);
        });
        return () => observer.disconnect();
    }, []);

    return (
        <nav className="dl-toc">
            <div className="dl-toc__title">On this page</div>
            <ul className="dl-toc__list">
                {TOC_ITEMS.map((t) => (
                    <li key={t.id}>
                        <a
                            href={`#${t.id}`}
                            className={active === t.id ? 'dl-toc__link dl-toc__link--active' : 'dl-toc__link'}
                        >
                            {t.label}
                        </a>
                    </li>
                ))}
            </ul>
        </nav>
    );
}

/* ────────────────────────────────────────────────────────────────
 *  Fallback data (when GitHub API is unavailable)
 * ──────────────────────────────────────────────────────────────── */

const FALLBACK_DEV: GitHubRelease = {
    tag_name: 'dev_build',
    name: 'Development Builds',
    published_at: new Date().toISOString(),
    html_url: 'https://github.com/mne-tools/mne-cpp/releases/tag/dev_build',
    target_commitish: '',
    assets: [],
};

const FALLBACK_STABLE: GitHubRelease = {
    tag_name: 'v0.1.9',
    name: 'v0.1.9',
    published_at: '2021-03-02T00:00:00Z',
    html_url: 'https://github.com/mne-tools/mne-cpp/releases/tag/v0.1.9',
    target_commitish: 'b00fbba',
    assets: [],
};

/* ────────────────────────────────────────────────────────────────
 *  Page
 * ──────────────────────────────────────────────────────────────── */

export default function Download(): JSX.Element {
    const [devRelease, setDevRelease] = useState<GitHubRelease | null>(null);
    const [stableRelease, setStableRelease] = useState<GitHubRelease | null>(null);
    const [devSha, setDevSha] = useState<string>('');
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        async function fetchData() {
            try {
                const [devRes, stableRes] = await Promise.all([
                    fetch(`${GH_API}/tags/dev_build`),
                    fetch(`${GH_API}/latest`),
                ]);
                if (devRes.ok) {
                    const dev: GitHubRelease = await devRes.json();
                    setDevRelease(dev);
                    try {
                        const tagRes = await fetch(
                            'https://api.github.com/repos/mne-tools/mne-cpp/git/ref/tags/dev_build',
                        );
                        if (tagRes.ok) {
                            const tagRef = await tagRes.json();
                            let sha = tagRef?.object?.sha ?? dev.target_commitish;
                            // Annotated tags: the ref points to a tag object, not a commit.
                            // Dereference it to get the actual commit SHA.
                            if (tagRef?.object?.type === 'tag' && tagRef.object.url) {
                                try {
                                    const derefRes = await fetch(tagRef.object.url);
                                    if (derefRes.ok) {
                                        const tagObj = await derefRes.json();
                                        if (tagObj?.object?.sha) {
                                            sha = tagObj.object.sha;
                                        }
                                    }
                                } catch { /* ignore dereference failure */ }
                            }
                            setDevSha(sha);
                        }
                    } catch { /* ignore */ }
                } else {
                    setDevRelease(FALLBACK_DEV);
                }
                if (stableRes.ok) {
                    setStableRelease(await stableRes.json());
                } else {
                    setStableRelease(FALLBACK_STABLE);
                }
            } catch {
                setDevRelease(FALLBACK_DEV);
                setStableRelease(FALLBACK_STABLE);
            } finally {
                setLoading(false);
            }
        }
        fetchData();
    }, []);

    return (
        <Layout title="Download" description="Download MNE-CPP binaries, development builds, and sample data">

            {/* ── Hero ── */}
            <header className="dl-hero">
                <div className="container">
                    <Heading as="h1">Download</Heading>
                    <p className="dl-hero__sub">
                        Installers, pre-built binaries, and development builds for Windows, macOS, and Linux.
                    </p>
                </div>
            </header>

            <main className="container margin-vert--lg">
                <div className="row">
                    <div className="col col--9">

                        {/* ──────────────────────────────────────────
                 *  1 · Releases — Stable & Development side-by-side
                 * ────────────────────────────────────────── */}
                        <div id="releases" className="row dl-row-gap">

                            {/* ── Stable ── */}
                            <div className="col col--6">
                                <section className="dl-card dl-card--full">
                                    <Heading as="h2">Latest Stable Release</Heading>
                                    <p className="dl-card__desc">
                                        Officially tested and tagged. Recommended for
                                        production use and reproducible research.
                                    </p>

                                    {loading ? <Skeleton /> : stableRelease && (
                                        <>
                                            <ReleaseMeta release={stableRelease} commitSha={stableRelease.target_commitish} />

                                            {stableRelease.assets.length > 0 && (
                                                <>
                                                    <AssetList assets={stableRelease.assets} platforms={STABLE_DYNAMIC} heading="Dynamic Binaries" />
                                                    <p className="dl-variant-note">Shared libraries. Smaller download. Full plugin support.</p>
                                                    <AssetList assets={stableRelease.assets} platforms={STABLE_STATIC} heading="Static Binaries" />
                                                    <p className="dl-variant-note">Self-contained. No external dependencies. No plugin support.</p>
                                                </>
                                            )}

                                            <p className="dl-links">
                                                <a href={stableRelease.html_url} target="_blank" rel="noopener noreferrer">
                                                    Release notes
                                                </a>
                                                <span className="dl-links__sep">·</span>
                                                <a href="https://github.com/mne-tools/mne-cpp/releases" target="_blank" rel="noopener noreferrer">
                                                    All releases
                                                </a>
                                            </p>
                                        </>
                                    )}
                                </section>
                            </div>

                            {/* ── Development ── */}
                            <div className="col col--6">
                                <section className="dl-card dl-card--full">
                                    <Heading as="h2">Development Builds</Heading>
                                    <p className="dl-card__desc">
                                        Automated nightly builds from the{' '}
                                        <a href="https://github.com/mne-tools/mne-cpp/tree/v2.0-dev" target="_blank" rel="noopener noreferrer">
                                            v2.0-dev
                                        </a>{' '}
                                        branch. Latest features and fixes; may be less stable.
                                    </p>

                                    {loading ? <Skeleton /> : devRelease && (
                                        <>
                                            <ReleaseMeta release={devRelease} commitSha={devSha || devRelease.target_commitish} />

                                            {devRelease.assets.length > 0 && (
                                                <>
                                                    <AssetList assets={devRelease.assets} platforms={INSTALLER_PLATFORMS} heading="Installers" />
                                                    <AssetList assets={devRelease.assets} platforms={DEV_DYNAMIC} heading="Dynamic Binaries" />
                                                    <p className="dl-variant-note">Shared libraries. Smaller download. Full plugin support.</p>
                                                    <AssetList assets={devRelease.assets} platforms={DEV_STATIC} heading="Static Binaries" />
                                                    <p className="dl-variant-note">Self-contained. No external dependencies. No plugin support.</p>
                                                    <AssetList assets={devRelease.assets} platforms={DEV_WASM} heading="WebAssembly" />
                                                </>
                                            )}

                                            <p className="dl-links">
                                                <a href={devRelease.html_url} target="_blank" rel="noopener noreferrer">
                                                    View all assets on GitHub
                                                </a>
                                            </p>
                                        </>
                                    )}
                                </section>
                            </div>
                        </div>

                        {/* ──────────────────────────────────────────
                 *  3 · Browser-Based Applications (WebAssembly)
                 * ────────────────────────────────────────── */}
                        <section id="browser-apps" className="dl-card">
                            <Heading as="h2">
                                Browser-Based Applications{' '}
                                <span className="dl-badge dl-badge--experimental">Experimental</span>
                            </Heading>
                            <p className="dl-card__desc">
                                Run MNE-CPP applications directly in your web browser using
                                WebAssembly — no installation required. This feature is experimental
                                and under active development. The Wasm module is downloaded once and
                                executed entirely inside your browser's sandboxed runtime.
                            </p>

                            <div className="dl-privacy">
                                <strong>Privacy.</strong>{' '}
                                No files are uploaded to any cloud service or remote server.
                                All computation happens locally on your machine; your data never
                                leaves your device.
                            </div>

                            <table className="dl-table">
                                <thead>
                                    <tr>
                                        <th>Application</th>
                                        <th>Description</th>
                                        <th className="dl-table__right">Launch</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <tr>
                                        <td>MNE Analyze</td>
                                        <td>Visualization, filtering, averaging, source localization</td>
                                        <td className="dl-table__right">
                                            <a
                                                href="https://mne-cpp.github.io/wasm/mne_analyze.html"
                                                target="_blank"
                                                rel="noopener noreferrer"
                                                className="button button--sm button--outline button--primary"
                                            >
                                                Open
                                            </a>
                                        </td>
                                    </tr>
                                    <tr>
                                        <td>MNE Anonymize</td>
                                        <td>Remove or replace personally identifiable information from FIFF files</td>
                                        <td className="dl-table__right">
                                            <a
                                                href="https://mne-cpp.github.io/wasm/mne_anonymize.html"
                                                target="_blank"
                                                rel="noopener noreferrer"
                                                className="button button--sm button--outline button--primary"
                                            >
                                                Open
                                            </a>
                                        </td>
                                    </tr>
                                </tbody>
                            </table>

                            <div className="dl-experimental-note">
                                <strong>Experimental.</strong>{' '}
                                These browser-based applications are provided as a technology preview.
                                Performance depends on your local hardware and some functionality may
                                be limited compared to native builds. Requires a modern browser with
                                WebAssembly support (Chrome, Firefox, Edge, Safari).
                            </div>
                        </section>

                        {/* ──────────────────────────────────────────
                 *  4 · Sample Data
                 * ────────────────────────────────────────── */}
                        <section id="sample-data" className="dl-card">
                            <Heading as="h2">Sample Data</Heading>
                            <p className="dl-card__desc">
                                The MNE sample dataset contains auditory and visual MEG/EEG recordings
                                together with structural MRI data. It is used across all MNE tutorials
                                and examples.
                            </p>

                            <table className="dl-table">
                                <thead>
                                    <tr>
                                        <th>Dataset</th>
                                        <th>Description</th>
                                        <th className="dl-table__right">Link</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <tr>
                                        <td>MNE-sample-data</td>
                                        <td>Auditory and visual MEG/EEG with MRI</td>
                                        <td className="dl-table__right">
                                            <a
                                                href="https://osf.io/86qa2/download"
                                                target="_blank"
                                                rel="noopener noreferrer"
                                                className="button button--sm button--outline button--primary"
                                            >
                                                Download (~1.5 GB)
                                            </a>
                                        </td>
                                    </tr>
                                </tbody>
                            </table>

                            <p className="dl-hint">
                                Extract the archive to{' '}
                                <code>resources/data/MNE-sample-data</code>{' '}
                                relative to the MNE-CPP directory.
                            </p>
                        </section>

                        {/* ──────────────────────────────────────────
                 *  5 · Getting Started
                 * ────────────────────────────────────────── */}
                        <section id="quick-start" className="dl-card">
                            <Heading as="h2">Quick Start</Heading>

                            <div className="row">
                                <div className="col col--6">
                                    <div className="dl-sub-card">
                                        <Heading as="h3">Pre-Built Binaries</Heading>
                                        <ol className="dl-steps">
                                            <li>Download the installer or archive for your platform.</li>
                                            <li>Run the installer or extract the archive.</li>
                                            <li>Launch <code>mne_analyze</code> or <code>mne_scan</code>.</li>
                                            <li>Optionally load the sample data above.</li>
                                        </ol>
                                        <Link to="/docs/manual/intro" className="button button--outline button--primary button--sm">
                                            User Manual
                                        </Link>
                                    </div>
                                </div>

                                <div className="col col--6">
                                    <div className="dl-sub-card">
                                        <Heading as="h3">Build from Source</Heading>
                                        <p className="dl-hint" style={{ marginBottom: '0.75rem' }}>
                                            Requires CMake, Qt 6, and Eigen 3. Supports GCC, Clang, and MSVC.
                                        </p>
                                        <pre className="dl-code"><code>{`git clone https://github.com/mne-tools/mne-cpp.git
cd mne-cpp && mkdir build && cd build
cmake .. && cmake --build .`}</code></pre>
                                        <Link to="/docs/development/buildguide-cmake" className="button button--outline button--primary button--sm">
                                            Build Guide
                                        </Link>
                                    </div>
                                </div>
                            </div>
                        </section>

                        {/* ──────────────────────────────────────────
                 *  6 · Verifying Downloads
                 * ────────────────────────────────────────── */}
                        <section id="verifying" className="dl-card">
                            <Heading as="h2">Verifying Downloads</Heading>
                            <p className="dl-card__desc">
                                Verify download integrity with SHA-256. Compare the output against
                                the checksum listed on the corresponding{' '}
                                <a href="https://github.com/mne-tools/mne-cpp/releases" target="_blank" rel="noopener noreferrer">
                                    GitHub release page
                                </a>.
                            </p>
                            <pre className="dl-code"><code>{`# macOS / Linux
shasum -a 256 <downloaded-file>

# Windows (PowerShell)
Get-FileHash <downloaded-file> -Algorithm SHA256`}</code></pre>
                        </section>

                        {/* ──────────────────────────────────────────
                 *  7 · System Requirements
                 * ────────────────────────────────────────── */}
                        <section id="requirements" className="dl-card">
                            <Heading as="h2">System Requirements</Heading>

                            <table className="dl-table">
                                <thead>
                                    <tr>
                                        <th>Platform</th>
                                        <th>Minimum Version</th>
                                        <th>Architecture</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <tr>
                                        <td>Windows</td>
                                        <td>Windows 10 or later</td>
                                        <td>x86_64</td>
                                    </tr>
                                    <tr>
                                        <td>macOS</td>
                                        <td>macOS 12 (Monterey) or later</td>
                                        <td>ARM64 / x86_64</td>
                                    </tr>
                                    <tr>
                                        <td>Linux</td>
                                        <td>Ubuntu 20.04 or equivalent</td>
                                        <td>x86_64</td>
                                    </tr>
                                    <tr>
                                        <td>Browser (WebAssembly)</td>
                                        <td>Chrome 89+, Firefox 89+, Safari 15+</td>
                                        <td>Any</td>
                                    </tr>
                                </tbody>
                            </table>
                        </section>

                        {/* ── License footer ── */}
                        <p className="dl-license">
                            MNE-CPP is released under the{' '}
                            <a href="https://github.com/mne-tools/mne-cpp/blob/main/LICENSE" target="_blank" rel="noopener noreferrer">
                                BSD 3-Clause License
                            </a>
                            . Free for academic and commercial use.
                        </p>

                    </div>{/* end col--9 */}

                    {/* ── Right sidebar TOC ── */}
                    <div className="col col--3 dl-toc-col">
                        <PageToc />
                    </div>
                </div>{/* end row */}
            </main>
        </Layout>
    );
}

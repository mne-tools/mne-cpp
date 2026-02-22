import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import Link from '@docusaurus/Link';

export default function Download(): JSX.Element {
    return (
        <Layout title="Download" description="Download MNE-CPP binaries and source code">
            <div className="container margin-vert--lg">
                <Heading as="h1">Download MNE-CPP</Heading>

                <div className="row margin-top--lg">
                    <div className="col col--6">
                        <div className="feature-card">
                            <Heading as="h2">🚀 Stable Release (v0.1.9)</Heading>
                            <p>Latest stable binaries for all platforms.</p>
                            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                <a href="https://github.com/mne-tools/mne-cpp/releases/tag/v0.1.9" className="button button--primary button--block">
                                    Windows (x86_64)
                                </a>
                                <a href="https://github.com/mne-tools/mne-cpp/releases/tag/v0.1.9" className="button button--primary button--block">
                                    Linux (x86_64)
                                </a>
                                <a href="https://github.com/mne-tools/mne-cpp/releases/tag/v0.1.9" className="button button--primary button--block">
                                    macOS (x86_64)
                                </a>
                            </div>
                        </div>
                    </div>

                    <div className="col col--6">
                        <div className="feature-card">
                            <Heading as="h2">🔧 Development Builds</Heading>
                            <p>Latest builds from the development branch (v2.0-dev).</p>
                            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                <a href="https://github.com/mne-tools/mne-cpp/releases/tag/dev_build" className="button button--secondary button--block">
                                    Dev Builds (All Platforms)
                                </a>
                                <a href="https://mne-cpp.github.io/wasm/" className="button button--outline button--secondary button--block">
                                    Try in Browser (WebAssembly)
                                </a>
                            </div>
                        </div>
                    </div>
                </div>

                <div className="margin-top--lg">
                    <Heading as="h2">Quick Start</Heading>
                    <ol>
                        <li>Download the archive for your platform</li>
                        <li>Extract the archive</li>
                        <li>Run the application (e.g., <code>mne_analyze</code> or <code>mne_scan</code>)</li>
                    </ol>
                    <p>
                        For building from source, see the{' '}
                        <Link to="/docs/development/buildguide-cmake">Build Guide</Link>.
                    </p>
                </div>

                <div className="margin-top--lg">
                    <Heading as="h2">Sample Data</Heading>
                    <p>
                        To try out MNE-CPP, download the{' '}
                        <a href="https://osf.io/86qa2/download" target="_blank" rel="noopener noreferrer">
                            MNE-Sample-Data set
                        </a>{' '}
                        and extract it to <code>resources/data/MNE-sample-data</code>.
                    </p>
                </div>
            </div>
        </Layout>
    );
}

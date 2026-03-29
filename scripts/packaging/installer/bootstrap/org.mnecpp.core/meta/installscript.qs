// installscript.qs -- Core MNE-CPP installer controller
//
// The online installer keeps the installer binary lightweight and fetches the
// selected payload from a remote Qt IFW repository during installation.

var binaryVariantPage = null;

var releaseTag = "@MNE_CPP_RELEASE_TAG@";
var repositoryUrl = "@MNE_CPP_REPOSITORY_URL@";
var defaultVariant = "dynamic";
var dynamicComponentId = "org.mnecpp.payload.dynamic";
var staticComponentId = "org.mnecpp.payload.static";
var sampleDataComponentId = "org.mnecpp.sampledata";
var mnePythonComponentId = "org.mnecpp.mnepython";
var pathConfigComponentId = "org.mnecpp.pathconfig";

var guiApps = ["mne_scan", "mne_analyze", "mne_browse", "mne_inspect"];

function pathSeparator()
{
    if (systemInfo.kernelType === "winnt") {
        return "\\";
    }

    return "/";
}

function joinPath(base, leaf)
{
    if (base === "") {
        return leaf;
    }

    var sep = pathSeparator();
    if (base.charAt(base.length - 1) === "/" || base.charAt(base.length - 1) === "\\") {
        return base + leaf;
    }

    return base + sep + leaf;
}

function selectedVariant()
{
    var envVariant = installer.environmentVariable("MNE_CPP_VARIANT");
    if (envVariant === "dynamic" || envVariant === "static") {
        return envVariant;
    }

    if (binaryVariantPage) {
        var radioStatic = binaryVariantPage.findChild("radioStatic");
        if (radioStatic && radioStatic.checked) {
            return "static";
        }
    }

    return defaultVariant;
}

function prettyVariantName(variant)
{
    if (variant === "static") {
        return "Static";
    }

    return "Dynamic";
}

function platformName()
{
    if (systemInfo.productType === "osx") {
        return "macOS";
    }

    if (systemInfo.kernelType === "winnt") {
        return "Windows";
    }

    return "Linux";
}

function shouldSkipDesktopIntegration()
{
    return installer.environmentVariable("MNE_CPP_CLI_ONLY") === "1";
}

function setComponentSelection(componentId, shouldSelect)
{
    if (shouldSelect) {
        installer.selectComponent(componentId);
    } else {
        installer.deselectComponent(componentId);
    }
}

function applySelectedOptions()
{
    var variant = selectedVariant();

    setComponentSelection(dynamicComponentId, variant === "dynamic");
    setComponentSelection(staticComponentId, variant === "static");

    if (binaryVariantPage) {
        var sampleDataCheck = binaryVariantPage.findChild("sampleDataCheck");
        if (sampleDataCheck) {
            setComponentSelection(sampleDataComponentId, sampleDataCheck.checked);
        }

        var mnePythonCheck = binaryVariantPage.findChild("mnePythonCheck");
        if (mnePythonCheck) {
            setComponentSelection(mnePythonComponentId, mnePythonCheck.checked);
        }

        var pathConfigCheck = binaryVariantPage.findChild("pathConfigCheck");
        if (pathConfigCheck) {
            setComponentSelection(pathConfigComponentId, pathConfigCheck.checked);
        }
    }

    installer.setValue("MNECPP_SELECTED_VARIANT", variant);
    installer.setValue("MNECPP_RELEASE_TAG", releaseTag);
    installer.setValue("MNECPP_REPOSITORY_URL", repositoryUrl);
    return true;
}

function ensureVariantPage()
{
    installer.addWizardPage(component, "BinaryVariantPage", QInstaller.TargetDirectory);
    binaryVariantPage = gui.pageWidgetByObjectName("DynamicBinaryVariantPage");
    if (!binaryVariantPage) {
        return;
    }

    binaryVariantPage.title = "Installation Options";
    binaryVariantPage.description =
        "Choose the MNE-CPP payload and optional add-ons to install from the online repository.";

    var rootLayout = new QVBoxLayout(binaryVariantPage);
    rootLayout.setSpacing(14);

    var card = new QFrame(binaryVariantPage);
    card.objectName = "MneHeroCard";
    var cardLayout = new QVBoxLayout(card);
    cardLayout.setSpacing(12);

    var kicker = new QLabel("Online installation", card);
    kicker.objectName = "MneKicker";
    cardLayout.addWidget(kicker);

    var title = new QLabel("Choose your MNE-CPP setup", card);
    title.objectName = "MneTitle";
    cardLayout.addWidget(title);

    var body = new QLabel(
        "MNE-CPP " + "@MNE_CPP_VERSION@" + " will be installed from the remote repository for " +
        platformName() + ". The native IFW progress page will then download and install the selected components.",
        card
    );
    body.objectName = "MneBody";
    body.wordWrap = true;
    cardLayout.addWidget(body);

    var repositorySummary = new QLabel(
        "Repository source: <tt>" + repositoryUrl + "</tt><br>Release channel: <b>" + releaseTag + "</b>",
        card
    );
    repositorySummary.objectName = "MneDetail";
    repositorySummary.wordWrap = true;
    cardLayout.addWidget(repositorySummary);

    var variantPanel = new QFrame(card);
    variantPanel.objectName = "MneOptionPanel";
    var variantLayout = new QVBoxLayout(variantPanel);
    variantLayout.setSpacing(10);

    var variantHeading = new QLabel("1. Select the binary payload", variantPanel);
    variantHeading.objectName = "MneStatusText";
    variantLayout.addWidget(variantHeading);

    var radioDynamic = new QRadioButton("Dynamic build", variantPanel);
    radioDynamic.objectName = "radioDynamic";
    radioDynamic.checked = true;
    variantLayout.addWidget(radioDynamic);

    var dynamicNote = new QLabel(
        "Shared libraries, smaller download, and the best default for most users.",
        variantPanel
    );
    dynamicNote.objectName = "MneDetail";
    dynamicNote.wordWrap = true;
    variantLayout.addWidget(dynamicNote);

    var radioStatic = new QRadioButton("Static build", variantPanel);
    radioStatic.objectName = "radioStatic";
    variantLayout.addWidget(radioStatic);

    var staticNote = new QLabel(
        "Larger download with fewer runtime dependencies. Useful for more self-contained deployments.",
        variantPanel
    );
    staticNote.objectName = "MneDetail";
    staticNote.wordWrap = true;
    variantLayout.addWidget(staticNote);

    cardLayout.addWidget(variantPanel);

    var extrasPanel = new QFrame(card);
    extrasPanel.objectName = "MneOptionPanel";
    var extrasLayout = new QVBoxLayout(extrasPanel);
    extrasLayout.setSpacing(10);

    var extrasHeading = new QLabel("2. Choose optional add-ons", extrasPanel);
    extrasHeading.objectName = "MneStatusText";
    extrasLayout.addWidget(extrasHeading);

    var pathConfigCheck = new QCheckBox("Add MNE-CPP tools to PATH", extrasPanel);
    pathConfigCheck.objectName = "pathConfigCheck";
    pathConfigCheck.checked = true;
    extrasLayout.addWidget(pathConfigCheck);

    var pathConfigNote = new QLabel(
        "Lets you run tools like mne_show_fiff from any terminal window after installation.",
        extrasPanel
    );
    pathConfigNote.objectName = "MneDetail";
    pathConfigNote.wordWrap = true;
    extrasLayout.addWidget(pathConfigNote);

    var sampleDataCheck = new QCheckBox("Download the MNE sample dataset", extrasPanel);
    sampleDataCheck.objectName = "sampleDataCheck";
    sampleDataCheck.checked = false;
    extrasLayout.addWidget(sampleDataCheck);

    var sampleDataNote = new QLabel(
        "Adds the tutorial dataset during installation. This is a separate large download.",
        extrasPanel
    );
    sampleDataNote.objectName = "MneDetail";
    sampleDataNote.wordWrap = true;
    extrasLayout.addWidget(sampleDataNote);

    var mnePythonCheck = new QCheckBox("Install MNE-Python via pip", extrasPanel);
    mnePythonCheck.objectName = "mnePythonCheck";
    mnePythonCheck.checked = false;
    extrasLayout.addWidget(mnePythonCheck);

    var mnePythonNote = new QLabel(
        "Uses your existing Python installation to add the MNE-Python package after the core files are installed.",
        extrasPanel
    );
    mnePythonNote.objectName = "MneDetail";
    mnePythonNote.wordWrap = true;
    extrasLayout.addWidget(mnePythonNote);

    cardLayout.addWidget(extrasPanel);
    rootLayout.addWidget(card);
    rootLayout.addStretch();
}

function Component()
{
    installer.setValue("MNECPP_RELEASE_TAG", releaseTag);
    installer.setValue("MNECPP_REPOSITORY_URL", repositoryUrl);

    if (installer.isInstaller()) {
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
        ensureVariantPage();
        installer.setValidatorForCustomPage(component,
            "BinaryVariantPage",
            "validateBinaryVariantPage");
        applySelectedOptions();
    }
}

Component.prototype.validateBinaryVariantPage = function()
{
    return applySelectedOptions();
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (shouldSkipDesktopIntegration()) {
        return;
    }

    var targetDir = installer.value("TargetDir");
    var variant = installer.value("MNECPP_SELECTED_VARIANT", selectedVariant());

    if (systemInfo.productType === "osx") {
        var appsFolder = "/Applications/MNE-CPP";

        component.addOperation("Execute",
            "/bin/mkdir", "-p", appsFolder,
            "UNDOEXECUTE",
            "/bin/rm", "-rf", appsFolder);

        for (var i = 0; i < guiApps.length; ++i) {
            var macApp = guiApps[i];
            var source = joinPath(targetDir, "bin/" + macApp + ".app");
            var link = joinPath(appsFolder, macApp + ".app");

            component.addOperation("Execute",
                "/bin/ln", "-sf", source, link,
                "UNDOEXECUTE",
                "/bin/rm", "-f", link);
        }

        return;
    }

    if (systemInfo.kernelType === "winnt") {
        for (var j = 0; j < guiApps.length; ++j) {
            var windowsApp = guiApps[j];
            var windowsPrettyName = windowsApp.replace("mne_", "MNE ").replace(/\b\w/g,
                function(c) { return c.toUpperCase(); });
            component.addOperation("CreateShortcut",
                targetDir + "\\bin\\" + windowsApp + ".exe",
                "@StartMenuDir@/" + windowsPrettyName + ".lnk",
                "workingDirectory=" + targetDir + "\\bin",
                "description=Launch " + prettyVariantName(variant) + " " + windowsPrettyName);
        }

        return;
    }

    var desktopDir = installer.environmentVariable("HOME") + "/.local/share/applications";
    component.addOperation("Mkdir", desktopDir);

    for (var k = 0; k < guiApps.length; ++k) {
        var linuxApp = guiApps[k];
        var prettyName = linuxApp.replace("mne_", "MNE ").replace(/\b\w/g,
            function(c) { return c.toUpperCase(); });
        var desktopFile = joinPath(desktopDir, "mnecpp-" + linuxApp + ".desktop");
        var desktop =
            "[Desktop Entry]\n" +
            "Type=Application\n" +
            "Name=" + prettyName + "\n" +
            "Exec=" + joinPath(targetDir, "bin/" + linuxApp) + "\n" +
            "Icon=" + joinPath(targetDir, "bin/" + linuxApp) + "\n" +
            "Terminal=false\n" +
            "Categories=Science;Education;\n" +
            "Comment=" + prettyVariantName(variant) + " MNE-CPP " + prettyName + "\n";

        component.addOperation("Execute",
            "/bin/bash", "-c",
            "cat > '" + desktopFile + "' <<'EOF'\n" + desktop + "EOF\nchmod +x '" + desktopFile + "'",
            "UNDOEXECUTE",
            "/bin/rm", "-f", desktopFile);
    }
}

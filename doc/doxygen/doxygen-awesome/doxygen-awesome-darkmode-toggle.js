// SPDX-License-Identifier: MIT
/**

Doxygen Awesome
https://github.com/jothepro/doxygen-awesome-css

Copyright (c) 2021 - 2025 jothepro
Modified: 3-state toggle (light → dark → system) to match Docusaurus

*/

class DoxygenAwesomeDarkModeToggle extends HTMLElement {
    // SVG icons – same as Docusaurus (Google Material Icons, Apache 2.0)
    static lightModeIcon = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24px" height="24px" fill="currentColor"><path d="M12,9c1.65,0,3,1.35,3,3s-1.35,3-3,3s-3-1.35-3-3S10.35,9,12,9 M12,7c-2.76,0-5,2.24-5,5s2.24,5,5,5s5-2.24,5-5 S14.76,7,12,7L12,7z M2,13l2,0c0.55,0,1-0.45,1-1s-0.45-1-1-1l-2,0c-0.55,0-1,0.45-1,1S1.45,13,2,13z M20,13l2,0c0.55,0,1-0.45,1-1 s-0.45-1-1-1l-2,0c-0.55,0-1,0.45-1,1S19.45,13,20,13z M11,2v2c0,0.55,0.45,1,1,1s1-0.45,1-1V2c0-0.55-0.45-1-1-1S11,1.45,11,2z M11,20v2c0,0.55,0.45,1,1,1s1-0.45,1-1v-2c0-0.55-0.45-1-1-1C11.45,19,11,19.45,11,20z M5.99,4.58c-0.39-0.39-1.03-0.39-1.41,0 c-0.39,0.39-0.39,1.03,0,1.41l1.06,1.06c0.39,0.39,1.03,0.39,1.41,0s0.39-1.03,0-1.41L5.99,4.58z M18.36,16.95 c-0.39-0.39-1.03-0.39-1.41,0c-0.39,0.39-0.39,1.03,0,1.41l1.06,1.06c0.39,0.39,1.03,0.39,1.41,0c0.39-0.39,0.39-1.03,0-1.41 L18.36,16.95z M19.42,5.99c0.39-0.39,0.39-1.03,0-1.41c-0.39-0.39-1.03-0.39-1.41,0l-1.06,1.06c-0.39,0.39-0.39,1.03,0,1.41 s1.03,0.39,1.41,0L19.42,5.99z M7.05,18.36c0.39-0.39,0.39-1.03,0-1.41c-0.39-0.39-1.03-0.39-1.41,0l-1.06,1.06 c-0.39,0.39-0.39,1.03,0,1.41s1.03,0.39,1.41,0L7.05,18.36z"/></svg>`
    static darkModeIcon = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24px" height="24px" fill="currentColor"><path d="M9.37,5.51C9.19,6.15,9.1,6.82,9.1,7.5c0,4.08,3.32,7.4,7.4,7.4c0.68,0,1.35-0.09,1.99-0.27C17.45,17.19,14.93,19,12,19 c-3.86,0-7-3.14-7-7C5,9.07,6.81,6.55,9.37,5.51z M12,3c-4.97,0-9,4.03-9,9s4.03,9,9,9s9-4.03,9-9c0-0.46-0.04-0.92-0.1-1.36 c-0.98,1.37-2.58,2.26-4.4,2.26c-2.98,0-5.4-2.42-5.4-5.4c0-1.81,0.89-3.42,2.26-4.4C12.92,3.04,12.46,3,12,3L12,3z"/></svg>`
    static systemModeIcon = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24px" height="24px" fill="currentColor"><path d="m12 21c4.971 0 9-4.029 9-9s-4.029-9-9-9-9 4.029-9 9 4.029 9 9 9zm4.95-13.95c1.313 1.313 2.05 3.093 2.05 4.95s-0.738 3.637-2.05 4.95c-1.313 1.313-3.093 2.05-4.95 2.05v-14c1.857 0 3.637 0.737 4.95 2.05z"/></svg>`

    static colorModeKey = "doxygen-color-mode"
    // Possible stored values: "light", "dark", null (= system)

    static _staticConstructor = function () {
        DoxygenAwesomeDarkModeToggle.applyColorMode()
        window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
            DoxygenAwesomeDarkModeToggle.applyColorMode()
        })
        document.addEventListener("visibilitychange", () => {
            if (document.visibilityState === 'visible') {
                DoxygenAwesomeDarkModeToggle.applyColorMode()
            }
        });
    }()

    /**
     * @returns "light", "dark", or null (system)
     */
    static get colorMode() {
        const stored = localStorage.getItem(DoxygenAwesomeDarkModeToggle.colorModeKey)
        if (stored === "light" || stored === "dark") return stored
        return null // system
    }

    static set colorMode(mode) {
        if (mode === "light" || mode === "dark") {
            localStorage.setItem(DoxygenAwesomeDarkModeToggle.colorModeKey, mode)
        } else {
            localStorage.removeItem(DoxygenAwesomeDarkModeToggle.colorModeKey)
        }
        DoxygenAwesomeDarkModeToggle.applyColorMode()
    }

    static get systemPreference() {
        return window.matchMedia('(prefers-color-scheme: dark)').matches
    }

    static get effectiveDarkMode() {
        const mode = DoxygenAwesomeDarkModeToggle.colorMode
        if (mode === "dark") return true
        if (mode === "light") return false
        return DoxygenAwesomeDarkModeToggle.systemPreference
    }

    static applyColorMode() {
        const dark = DoxygenAwesomeDarkModeToggle.effectiveDarkMode
        DoxygenAwesomeDarkModeToggle.darkModeEnabled = dark
        if (dark) {
            document.documentElement.classList.add("dark-mode")
            document.documentElement.classList.remove("light-mode")
        } else {
            document.documentElement.classList.remove("dark-mode")
            document.documentElement.classList.add("light-mode")
        }
    }

    /**
     * Cycle: light → dark → system → light → …
     * (same order as Docusaurus)
     */
    static getNextColorMode(current) {
        switch (current) {
            case null: return "light"
            case "light": return "dark"
            case "dark": return null
            default: return "light"
        }
    }

    static init() {
        $(function () {
            $(document).ready(function () {
                const toggleButton = document.createElement('doxygen-awesome-dark-mode-toggle')
                toggleButton.updateIcon()

                window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
                    toggleButton.updateIcon()
                })
                document.addEventListener("visibilitychange", () => {
                    if (document.visibilityState === 'visible') {
                        toggleButton.updateIcon()
                    }
                });

                $(document).ready(function () {
                    document.getElementById("MSearchBox").parentNode.appendChild(toggleButton)
                })
                $(window).resize(function () {
                    document.getElementById("MSearchBox").parentNode.appendChild(toggleButton)
                })
            })
        })
    }

    constructor() {
        super();
        this.onclick = this.toggleDarkMode
    }

    toggleDarkMode() {
        const next = DoxygenAwesomeDarkModeToggle.getNextColorMode(DoxygenAwesomeDarkModeToggle.colorMode)
        DoxygenAwesomeDarkModeToggle.colorMode = next
        this.updateIcon()
    }

    updateIcon() {
        const mode = DoxygenAwesomeDarkModeToggle.colorMode
        if (mode === "light") {
            this.innerHTML = DoxygenAwesomeDarkModeToggle.lightModeIcon
            this.title = "Light mode – click to switch to dark mode"
        } else if (mode === "dark") {
            this.innerHTML = DoxygenAwesomeDarkModeToggle.darkModeIcon
            this.title = "Dark mode – click to switch to system mode"
        } else {
            this.innerHTML = DoxygenAwesomeDarkModeToggle.systemModeIcon
            this.title = "System mode – click to switch to light mode"
        }
    }
}

customElements.define("doxygen-awesome-dark-mode-toggle", DoxygenAwesomeDarkModeToggle);

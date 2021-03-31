---
title: Changelog
parent: Download
nav_exclude: true
---

# Changelog

## Version 0.1.9 - 2021/03/02

### Applications

MNE Analyze
\begin{itemize}
	\item Handle removal and deletion of child item models
	\item Add event for model removal
	\item Changed event documentation format
	\item Gave AnalyzeData a communicator to send events
	\item Implemented clearing of views in plugins that use models
	\item Data Loader now saves last folder data was loaded from.
	\item Code cleanup to fix some compiler warnings and memory leaks
	\item Show file and filter info in signal viewer.
\end{itemize}

MNE Anonymize
\begin{itemize}
	\item Fix bug when anonymizing dates
	\item Update date command line option
	\item Fix link to documentation web page
	\item Use QDate instead of QDateTime when referring to birthday date
	\item add console to CONFIG in pro file
\end{itemize}

MNE Scan
\begin{itemize}
	\item Change saving to file to account for calibration values when saving data
\end{itemize}

EDF-To-Fiff Converter
\begin{itemize}
	\item Added edf to fiff converter command line application
\end{itemize}

### API librariers

Disp
\begin{itemize}
	\item Added clearView() function to AbstractView
	\item Change default loaded values for ScalingView and FiffRawViewSettings
	\item Made FilterSettingsView and FilterDesignView reflect filter parameter changes made in each other
	\item Fix updating filter parameters in FilterDesignView after loading filter from file
	\item Update scaling of FilterDesignView plotting and removed scroll bars.
	\item Made updateFilterPlot public in FilterDesignView
	\item Text color in the FilterPlotScene class is now dependant on the FilterDesignView colors, dependent on the Qt stylesheet
\end{itemize}


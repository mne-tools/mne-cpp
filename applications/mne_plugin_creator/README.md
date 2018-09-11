# MNE Plugin Creator

MNE Plugin Creator is a tool to automate the creation of new MNE-CPP plugins. It consists of a simple CLI that polls the user for information and then generates a project to help them get started. It also automatically creates a testframe for the new plugin to encourage proper unit testing practices.

## Motivation

Learning the Qt ecosystem is challenging and can be frustrating for new users who just want to implement a simple plugin. Even just copying the DummyPlugin and renaming it is non-trivial and surprisingly time consuming. Providing a CLI plugin creation tool the likes of Javascript's `npm` or Swift's `spm` will significantly lower MNE-CPP's barrier to entry for new users and increase its adoption.

## Usage

Presently `mne_plugin_creator` is intended to be run from within the Qt Creator IDE. Set it as the active project and run it to be presented with the CLI. You will be asked for basic information about the plugin you want to create.

When complete, Qt Creator should automatically reload the MNE-CPP workspace with your plugin included. If a pop message appears asking you if you want to save, close, or reload any files, choose the reload option.

Presently only MNE Scan's `IPlugin` and `IAlgorithm` are supported. The templates used for each are based on Christoph's DummyPlugin.

## Development

The plugin creator is designed to be extensible so that it can support more types of plugins and apps as the MNE-CPP ecosystem expands.

The basic principle behind `mne_plugin_creator` is that there are a number of plain text template files stored in it's `templates/` directory. These templates all contain fields that are marked with double brackets, such as `{{author}}` and `{{header_file_name}}`. When you create a new plugin, what actually happens is that the template files are duplicated, renamed, and the information you provided to the CLI is substituted into these fields.

Modifying the details of the generated projects is as simple as modifying the template files.

To extend `mne_plugin_creator`, you will need to start by writing a new parser class to collect information from the user. Subclass `IInputParser` and it's abstract methods. It has a number of utility function for performing common tasks like displaying options and validating input. `MNEScanInputParser` can serve as a reference. Once you've created your parser, add it to `AppInputParser`'s `parseUserInput` method.

Next you need to write an `IPluginCreator` subclass and implement its abstract methods to specify where the templates are and where to copy them to, as well as how Qt's .pro files need to be adjusted. Optionally, you can override the default implementation of the testframe creation method to provide a more detailed test frame boilerplate. `MNEScanPluginCreator` may be a valuable reference.

## Future Work

1.  Setting up unit tests for MNE Scan plugins is very difficult. We should provide a template that help users get access to their plugins inside a test frame. The present template just provides access to basic Qt classes, which isn't actually very useful at all.

2.  There is no support for MNE Analyze plugins yet. `IInputParser` and `IPluginCreator` classes should be written for MNE Analyze.

3.  We should update `mne_plugin_creator`'s .pro file to put the build output in the MNE binary folder. The challenge is ensuring that the correct relative path to the source code folders are provided to the plugin creator classes regardless of which directory the tool is run from. Presently we assume it will be run from the default build directory and have hard coded relative paths from that directory.

    * See `MNEScanPluginCreator`'s constructor.

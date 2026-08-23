# Translations

The Dimecoin Core project has been designed to support multiple localisations. This makes adding new phrases, and completely new languages easily achievable. For shared strings, Dimecoin Core currently reuses **Bitcoin Core's Transifex project as an upstream reference**. There is no separate, verified Dimecoin translation platform yet, and Dimecoin does not currently run automated translation sync; Dimecoin-specific translation infrastructure is a future effort. This document describes the verified upstream workflow only.

### Helping to translate (using Transifex)

The Bitcoin Transifex project monitors the upstream Bitcoin Core repository; for Dimecoin this is a **reference** for shared strings only. Dimecoin does not currently run automated translation monitoring or sync.

Multiple language support is critical in assisting Dimecoin’s global adoption, and growth. One of Dimecoin's greatest strengths is cross-border money transfers, any help making that easier is greatly appreciated.

Bitcoin's Transifex project ([upstream reference](https://www.transifex.com/projects/p/bitcoin/)) is useful for common strings, but it is **not** Dimecoin's translation platform. Dimecoin-specific translations are not yet automated; coordinate with the maintainers if you want to stand up a Dimecoin translation project.

### Writing code with translations

We use automated scripts to help extract translations in both Qt, and non-Qt source files. It is rarely necessary to manually edit the files in `src/qt/locale/`. The translation source files must adhere to the following format:
`bitcoin_xx_YY.ts or bitcoin_xx.ts`

`src/qt/locale/bitcoin_en.ts` is treated in a special way. It is used as the source for all other translations. Whenever a string in the source code is changed, this file must be updated to reflect those changes. A custom script is used to extract strings from the non-Qt parts. This script makes use of `gettext`, so make sure that utility is installed (ie, `apt-get install gettext` on Ubuntu/Debian). Once this has been updated, `lupdate` (included in the Qt SDK) is used to update `bitcoin_en.ts`.

To automatically regenerate the `bitcoin_en.ts` file, run the following commands:

```sh
cd src/
make translate
```

`contrib/bitcoin-qt.pro` takes care of generating `.qm` (binary compiled) files from `.ts` (source files) files. It’s mostly automated, and you shouldn’t need to worry about it.

**Example Qt translation**

```cpp
QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
```

### Creating a pull-request

For general PRs, you shouldn’t include any updates to the translation source files. They will be updated periodically, primarily around pre-releases, allowing time for any new phrases to be translated before public releases. This is also important in avoiding translation related merge conflicts.

When an updated source file is merged into the upstream Bitcoin Core repo, Transifex will automatically detect it (although it can take several hours). For Dimecoin this is a reference workflow only; Dimecoin does not currently run this automation.

To create the pull-request, use the following commands:

```
git add src/qt/bitcoinstrings.cpp src/qt/locale/bitcoin_en.ts
git commit
```

### Creating a Transifex account

Visit the [Transifex Signup](https://www.transifex.com/signup/) page to create an account. Take note of your username and password, as they will be required to configure the command-line tool.

The Bitcoin translation project at [https://www.transifex.com/projects/p/bitcoin/](https://www.transifex.com/projects/p/bitcoin/) is the upstream reference used by Dimecoin; Dimecoin does not currently have its own Transifex project.

### Installing the Transifex client command-line tool

The client is used to fetch updated translations. If you are having problems, or need more details, see [https://docs.transifex.com/client/installing-the-client](https://docs.transifex.com/client/installing-the-client)

`pip install transifex-client`

Setup your Transifex client config as follows. Please _ignore the token field_.

```ini
nano ~/.transifexrc

[https://www.transifex.com]
hostname = https://www.transifex.com
password = PASSWORD
token =
username = USERNAME
```

The `.tx/config` file shipped in the repo currently targets the **Bitcoin upstream** project (not a Dimecoin project); you shouldn’t normally need to change it. Dimecoin translation automation is not yet configured.

### Synchronising translations

To assist in updating translations, we have created a script to help.

1. `python contrib/devtools/update-translations.py`
2. Update `src/qt/bitcoin_locale.qrc` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/<file alias="\2">locale\/\1.qm<\/file>/'`
3. Update `src/Makefile.qt.include` manually or via
   `ls src/qt/locale/*ts|xargs -n1 basename|sed 's/\(bitcoin_\(.*\)\).ts/  qt\/locale\/\1.ts \\/'`
4. `git add` new translations from `src/qt/locale/`

**Do not directly download translations** one by one from the Transifex website, as we do a few post-processing steps before committing the translations.

### Handling Plurals (in source files)

When new plurals are added to the source file, it's important to do the following steps:

1. Open `bitcoin_en.ts` in Qt Linguist (included in the Qt SDK)
2. Search for `%n`, which will take you to the parts in the translation that use plurals
3. Look for empty `English Translation (Singular)` and `English Translation (Plural)` fields
4. Add the appropriate strings for the singular and plural form of the base string
5. Mark the item as done (via the green arrow symbol in the toolbar)
6. Repeat from step 2, until all singular and plural forms are in the source file
7. Save the source file

### Translating a new language

To create a new language template, you will need to edit the languages manifest file `src/qt/bitcoin_locale.qrc` and add a new entry. Below is an example of the English language entry.

```xml
<qresource prefix="/translations">
    <file alias="en">locale/bitcoin_en.qm</file>
    ...
</qresource>
```

**Note:** that the language translation file **must end in `.qm`** (the compiled extension), and not `.ts`.

### Questions and general assistance

For reference, the upstream Bitcoin-Core translation maintainers include _tcatm, seone, Diapolo, wumpus and luke-jr_ (reachable in the Freenode IRC chatroom `irc.freenode.net #bitcoin-core-dev`). For Dimecoin-specific translation questions, use Dimecoin's own community channels.

If you are a translator, you should also subscribe to the mailing list, https://groups.google.com/forum/#!forum/bitcoin-translators. Announcements will be posted during application pre-releases to notify translators to check for updates.

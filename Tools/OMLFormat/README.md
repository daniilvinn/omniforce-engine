# OMLFormat - ONNX to JSON Converter Tool

<div align="center">

*A Python-based tool for converting ONNX neural network models to human-readable JSON format*

</div>

## 📋 Table of Contents

- [Overview](#overview)
- [✨ Features](#-features)
- [📋 Requirements](#-requirements)
- [🔧 Installation](#-installation)
- [🚀 Usage](#-usage)
- [📁 Output Files](#-output-files)
- [⚡ Performance Considerations](#-performance-considerations)
- [⚠️ Error Handling](#️-error-handling)
- [📊 Conversion Statistics](#-conversion-statistics)
- [🎯 Use Cases](#-use-cases)
- [🚧 Limitations](#-limitations)
- [🤝 Contributing](#-contributing)

## Overview

OMLFormat is a Python-based tool that converts ONNX (Open Neural Network Exchange) model files to JSON format. This tool preserves all model information including metadata, nodes, inputs, outputs, and initializers, making it easier to inspect, analyze, and work with neural network models in a human-readable format.

## ✨ Features

- **🔄 Complete Model Conversion**: Converts all ONNX model components including graphs, nodes, attributes, and tensors
- **💾 Flexible Weight Storage**: Option to embed weights in JSON or store them separately in binary files
- **🗜️ Weight Compression**: Optional DEFLATE compression for weight data to reduce file sizes
- **✅ Model Validation**: Built-in ONNX model validation before conversion
- **📋 Comprehensive Output**: Preserves all metadata, producer information, and model properties

## 📋 Requirements

- **🐍 Python 3.x**
- **📦 ONNX library** (`pip install onnx`)
- **🔢 NumPy** (`pip install numpy`)

## 🔧 Installation

1. **Install the required dependencies:**
   ```bash
   pip install onnx numpy
   ```

2. **The tool is ready to use from the `Tools/OMLFormat/` directory.**

## 🚀 Usage

### Basic Usage

```bash
python OMLFormat.py <input_onnx_file> [output_json_file]
```

### Command Line Arguments

| Argument | Description | Required |
|----------|-------------|----------|
| `input_file` | Path to the input ONNX file | ✅ Yes |
| `output_file` | Path to the output JSON file (optional) | ❌ No |

### Available Flags

| Flag | Description |
|------|-------------|
| `--pretty` | Pretty print the JSON output (default: True) |
| `--validate` | Validate the ONNX model before conversion |
| `--separate-weights` | Store weights in separate .nnp binary file instead of embedding in JSON |
| `--compress-weights` | Compress weights using DEFLATE algorithm (requires --separate-weights) |

## 📝 Usage Examples

### 🔄 Basic Conversion
Convert an ONNX model to JSON with default settings:
```bash
python OMLFormat.py model.onnx
```
This creates `model.json` in the same directory.

### 📁 Specify Output File
Convert with a custom output filename:
```bash
python OMLFormat.py model.onnx output.json
```

### 💾 Separate Weight Storage
Store weights in a separate binary file to reduce JSON size:
```bash
python OMLFormat.py --separate-weights model.onnx
```
This creates both `model.json` and `model.nnp` files.

### 🗜️ Compressed Weight Storage
Store weights separately with compression for smaller file sizes:
```bash
python OMLFormat.py --separate-weights --compress-weights model.onnx
```

### ✅ Model Validation
Validate the ONNX model before conversion:
```bash
python OMLFormat.py --validate model.onnx
```

### 🎯 Complete Example
Full conversion with validation, separate weights, and compression:
```bash
python OMLFormat.py --validate --separate-weights --compress-weights model.onnx
```

## 📁 Output Files

### 📄 JSON File Structure
The generated JSON file contains the complete ONNX model structure:

```json
{
  "ir_version": 8,
  "opset_import": [...],
  "producer_name": "onnx",
  "producer_version": "1.12.0",
  "domain": "",
  "model_version": 1,
  "doc_string": "",
  "graph": {
    "name": "model",
    "inputs": [...],
    "outputs": [...],
    "initializers": [...],
    "nodes": [...],
    "sparse_initializers": [...]
  },
  "metadata_props": [...]
}
```

### 💾 Weight Storage Options

1. **📄 Embedded Weights** (default): Weight data is included directly in the JSON file as arrays
2. **💾 Separate Weights** (`--separate-weights`): Weights are stored in a separate `.nnp` binary file
3. **🗜️ Compressed Weights** (`--compress-weights`): Weights are compressed using DEFLATE algorithm

## ⚡ Performance Considerations

- **🐘 Large Models**: For models with large weight tensors, use `--separate-weights` to avoid extremely large JSON files
- **🗜️ Compression**: Use `--compress-weights` for significant file size reduction (typically 2-4x smaller)
- **💾 Memory Usage**: Large models may require significant memory during conversion

## ⚠️ Error Handling

The tool provides detailed error messages for common issues:
- **📦 Missing ONNX library**
- **❌ Invalid ONNX file format**
- **🔒 File permission errors**
- **💾 Memory allocation failures**

## 📊 Conversion Statistics

After successful conversion, the tool displays:
- **📥 Input file size**
- **📤 Output file sizes** (JSON + weights if separate)
- **🗜️ Compression ratios**
- **📉 Overall file size reduction**

## 🎯 Use Cases

- **🔍 Model Inspection**: Analyze model architecture and parameters
- **🐛 Debugging**: Examine model structure and weights
- **📚 Documentation**: Generate human-readable model descriptions
- **📝 Version Control**: Store model information in text format
- **📊 Analysis**: Process model data with JSON tools and libraries

## 🚧 Limitations

- **💾 Memory**: Very large models may require significant memory during conversion
- **🔧 Complexity**: Some complex ONNX features may not be fully represented in JSON
- **📄 Binary Files**: Binary weight files are not human-readable

## 🤝 Contributing

This tool is part of the **Omniforce Engine** project. For issues or improvements, please refer to the main project documentation or (better) project's author.

---

<div align="center">

**Made with ❤️ by the Daniil Vinnik**

</div> 
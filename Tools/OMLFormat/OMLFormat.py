#!/usr/bin/env python3
"""
ONNX to JSON Converter Tool

This tool converts ONNX model files to JSON format, preserving all model information
including metadata, nodes, inputs, outputs, and initializers.

Usage:
    python onnx_to_json.py <input_onnx_file> [output_json_file]
    
Example:
    python onnx_to_json.py model.onnx
    python onnx_to_json.py model.onnx model.json
"""

import json
import sys
import os
import argparse
import zlib
from typing import Dict, Any, List, Union
import numpy as np

try:
    import onnx
    from onnx import numpy_helper
except ImportError:
    print("Error: ONNX library not found. Please install it using:")
    print("pip install onnx")
    sys.exit(1)


def convert_tensor_to_dict(tensor, weights_file=None, weights_offset=0, compress_weights=False) -> tuple[Dict[str, Any], int]:
    """Convert ONNX tensor to dictionary format."""
    tensor_dict = {
        "name": tensor.name,
        "data_type": str(tensor.data_type),
        "dims": list(tensor.dims),
        "raw_data_size": len(tensor.raw_data) if tensor.raw_data else 0
    }
    
    # Convert raw data to numpy array if possible
    if tensor.raw_data:
        try:
            np_array = numpy_helper.to_array(tensor)
            tensor_dict["shape"] = list(np_array.shape)
            
            if weights_file is not None:
                # Store weights in binary file and reference by offset
                data_bytes = np_array.tobytes()
                original_size = len(data_bytes)
                
                if compress_weights:
                    # Compress using DEFLATE
                    compressed_data = zlib.compress(data_bytes, level=6)
                    data_bytes = compressed_data
                    tensor_dict["compressed"] = True
                    tensor_dict["original_size"] = original_size
                else:
                    tensor_dict["compressed"] = False
                
                tensor_dict["weights_offset"] = weights_offset
                tensor_dict["weights_size"] = len(data_bytes)
                tensor_dict["data"] = None  # No data in JSON
                
                # Write to binary file
                weights_file.write(data_bytes)
                return tensor_dict, weights_offset + len(data_bytes)
            else:
                # Convert to list for JSON serialization (original behavior)
                tensor_dict["data"] = np_array.tolist()
                tensor_dict["shape"] = list(np_array.shape)
                return tensor_dict, weights_offset
                
        except Exception as e:
            tensor_dict["error"] = f"Failed to convert tensor data: {str(e)}"
            return tensor_dict, weights_offset
    else:
        tensor_dict["data"] = None
        tensor_dict["shape"] = None
        return tensor_dict, weights_offset


def convert_value_info_to_dict(value_info) -> Dict[str, Any]:
    """Convert ONNX ValueInfoProto to dictionary format."""
    return {
        "name": value_info.name,
        "type": {
            "tensor_type": {
                "elem_type": str(value_info.type.tensor_type.elem_type),
                "shape": {
                    "dim": [
                        {
                            "dim_value": dim.dim_value if dim.dim_value else None,
                            "dim_param": dim.dim_param if dim.dim_param else None
                        }
                        for dim in value_info.type.tensor_type.shape.dim
                    ]
                }
            }
        }
    }


def convert_attribute_to_dict(attr, weights_file=None, weights_offset=0, compress_weights=False) -> tuple[Dict[str, Any], int]:
    """Convert ONNX AttributeProto to dictionary format."""
    attr_dict = {
        "name": attr.name,
        "type": str(attr.type)
    }
    
    # Convert attribute value based on type
    if attr.type == onnx.AttributeProto.FLOAT:
        attr_dict["value"] = attr.f
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.INT:
        attr_dict["value"] = attr.i
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.STRING:
        attr_dict["value"] = attr.s.decode('utf-8') if isinstance(attr.s, bytes) else attr.s
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.TENSOR:
        tensor_dict, new_offset = convert_tensor_to_dict(attr.t, weights_file, weights_offset, compress_weights)
        attr_dict["value"] = tensor_dict
        return attr_dict, new_offset
    elif attr.type == onnx.AttributeProto.GRAPH:
        graph_dict, new_offset = convert_graph_to_dict(attr.g, weights_file, weights_offset, compress_weights)
        attr_dict["value"] = graph_dict
        return attr_dict, new_offset
    elif attr.type == onnx.AttributeProto.FLOATS:
        attr_dict["value"] = list(attr.floats)
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.INTS:
        attr_dict["value"] = list(attr.ints)
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.STRINGS:
        attr_dict["value"] = [s.decode('utf-8') if isinstance(s, bytes) else s for s in attr.strings]
        return attr_dict, weights_offset
    elif attr.type == onnx.AttributeProto.TENSORS:
        tensors = []
        current_offset = weights_offset
        for t in attr.tensors:
            tensor_dict, current_offset = convert_tensor_to_dict(t, weights_file, current_offset, compress_weights)
            tensors.append(tensor_dict)
        attr_dict["value"] = tensors
        return attr_dict, current_offset
    elif attr.type == onnx.AttributeProto.GRAPHS:
        graphs = []
        current_offset = weights_offset
        for g in attr.graphs:
            graph_dict, current_offset = convert_graph_to_dict(g, weights_file, current_offset, compress_weights)
            graphs.append(graph_dict)
        attr_dict["value"] = graphs
        return attr_dict, current_offset
    else:
        attr_dict["value"] = None
        return attr_dict, weights_offset


def convert_node_to_dict(node, weights_file=None, weights_offset=0, compress_weights=False) -> tuple[Dict[str, Any], int]:
    """Convert ONNX NodeProto to dictionary format."""
    attributes = []
    current_offset = weights_offset
    
    for attr in node.attribute:
        attr_dict, current_offset = convert_attribute_to_dict(attr, weights_file, current_offset, compress_weights)
        attributes.append(attr_dict)
    
    return {
        "name": node.name,
        "op_type": node.op_type,
        "domain": node.domain,
        "inputs": list(node.input),
        "outputs": list(node.output),
        "attributes": attributes
    }, current_offset


def convert_graph_to_dict(graph, weights_file=None, weights_offset=0, compress_weights=False) -> tuple[Dict[str, Any], int]:
    """Convert ONNX GraphProto to dictionary format."""
    current_offset = weights_offset
    
    # Convert inputs
    inputs = [convert_value_info_to_dict(input_info) for input_info in graph.input]
    
    # Convert outputs
    outputs = [convert_value_info_to_dict(output_info) for output_info in graph.output]
    
    # Convert initializers (weights)
    initializers = []
    for init in graph.initializer:
        init_dict, current_offset = convert_tensor_to_dict(init, weights_file, current_offset, compress_weights)
        initializers.append(init_dict)
    
    # Convert nodes
    nodes = []
    for node in graph.node:
        node_dict, current_offset = convert_node_to_dict(node, weights_file, current_offset, compress_weights)
        nodes.append(node_dict)
    
    # Convert sparse initializers
    sparse_initializers = []
    for sparse_init in graph.sparse_initializer:
        sparse_dict, current_offset = convert_tensor_to_dict(sparse_init, weights_file, current_offset, compress_weights)
        sparse_initializers.append(sparse_dict)
    
    return {
        "name": graph.name,
        "inputs": inputs,
        "outputs": outputs,
        "initializers": initializers,
        "nodes": nodes,
        "sparse_initializers": sparse_initializers
    }, current_offset


def convert_model_to_dict(model, weights_file=None, weights_offset=0, compress_weights=False) -> tuple[Dict[str, Any], int]:
    """Convert ONNX ModelProto to dictionary format."""
    graph_dict, final_offset = convert_graph_to_dict(model.graph, weights_file, weights_offset, compress_weights)
    
    model_dict = {
        "ir_version": model.ir_version,
        "opset_import": [
            {
                "domain": opset.domain,
                "version": opset.version
            }
            for opset in model.opset_import
        ],
        "producer_name": model.producer_name,
        "producer_version": model.producer_version,
        "domain": model.domain,
        "model_version": model.model_version,
        "doc_string": model.doc_string,
        "graph": graph_dict,
        "metadata_props": [
            {
                "key": prop.key,
                "value": prop.value
            }
            for prop in model.metadata_props
        ]
    }
    
    return model_dict, final_offset


def onnx_to_json(onnx_file_path: str, json_file_path: str = None, separate_weights: bool = False, compress_weights: bool = False) -> str:
    """
    Convert ONNX file to JSON format.
    
    Args:
        onnx_file_path: Path to the input ONNX file
        json_file_path: Path to the output JSON file (optional)
        separate_weights: If True, store weights in separate .nnp binary file
        compress_weights: If True, compress weights using DEFLATE algorithm
    
    Returns:
        Path to the created JSON file
    """
    try:
        # Load ONNX model
        print(f"Loading ONNX model from: {onnx_file_path}")
        model = onnx.load(onnx_file_path)
        
        # Determine output file paths
        if json_file_path is None:
            base_name = os.path.splitext(onnx_file_path)[0]
            json_file_path = f"{base_name}.json"
        
        weights_file_path = None
        weights_file = None
        
        if separate_weights:
            weights_file_path = json_file_path.replace('.json', '.nnp')
            compression_info = " (compressed)" if compress_weights else ""
            print(f"Creating weights file: {weights_file_path}{compression_info}")
            weights_file = open(weights_file_path, 'wb')
        
        try:
            # Convert to dictionary
            print("Converting ONNX model to JSON format...")
            model_dict, _ = convert_model_to_dict(model, weights_file, 0, compress_weights)
            
            # Write JSON file
            print(f"Writing JSON to: {json_file_path}")
            with open(json_file_path, 'w', encoding='utf-8') as f:
                json.dump(model_dict, f, indent=2, ensure_ascii=False)
            
            if separate_weights:
                print(f"Successfully converted {onnx_file_path} to {json_file_path} and {weights_file_path}")
            else:
                print(f"Successfully converted {onnx_file_path} to {json_file_path}")
            
            return json_file_path
            
        finally:
            if weights_file:
                weights_file.close()
        
    except Exception as e:
        print(f"Error converting ONNX file: {str(e)}")
        raise


def main():
    """Main function to handle command line arguments and execute conversion."""
    parser = argparse.ArgumentParser(
        description="Convert ONNX model files to JSON format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python OMLFormat.py model.onnx
  python OMLFormat.py model.onnx output.json
  python OMLFormat.py --separate-weights model.onnx
  python OMLFormat.py --separate-weights --compress-weights model.onnx
  python OMLFormat.py --validate --separate-weights --compress-weights model.onnx
        """
    )
    
    parser.add_argument(
        "input_file",
        help="Path to the input ONNX file"
    )
    
    parser.add_argument(
        "output_file",
        nargs="?",
        help="Path to the output JSON file (optional, defaults to input_file.json)"
    )
    
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty print the JSON output (default: True)"
    )
    
    parser.add_argument(
        "--validate",
        action="store_true",
        help="Validate the ONNX model before conversion"
    )
    
    parser.add_argument(
        "--separate-weights",
        action="store_true",
        help="Store weights in separate .nnp binary file instead of embedding in JSON"
    )
    
    parser.add_argument(
        "--compress-weights",
        action="store_true",
        help="Compress weights using DEFLATE algorithm (requires --separate-weights)"
    )
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.compress_weights and not args.separate_weights:
        print("Error: --compress-weights requires --separate-weights")
        sys.exit(1)
    
    # Check if input file exists
    if not os.path.exists(args.input_file):
        print(f"Error: Input file '{args.input_file}' does not exist.")
        sys.exit(1)
    
    try:
        # Validate ONNX model if requested
        if args.validate:
            print("Validating ONNX model...")
            model = onnx.load(args.input_file)
            onnx.checker.check_model(model)
            print("ONNX model validation passed.")
        
        # Convert ONNX to JSON
        output_path = onnx_to_json(args.input_file, args.output_file, args.separate_weights, args.compress_weights)
        
        # Print file size information
        input_size = os.path.getsize(args.input_file)
        output_size = os.path.getsize(output_path)
        
        print(f"\nConversion Summary:")
        print(f"Input file size:  {input_size:,} bytes")
        print(f"JSON file size:   {output_size:,} bytes")
        
        if args.separate_weights:
            weights_file_path = output_path.replace('.json', '.nnp')
            if os.path.exists(weights_file_path):
                weights_size = os.path.getsize(weights_file_path)
                total_output_size = output_size + weights_size
                print(f"Weights file size: {weights_size:,} bytes")
                print(f"Total output size: {total_output_size:,} bytes")
                
                if args.compress_weights:
                    # Calculate compression statistics
                    compression_ratio = weights_size / input_size if input_size > 0 else 1.0
                    print(f"Weights compression ratio: {compression_ratio:.2f}x")
                
                print(f"Overall compression ratio: {total_output_size/input_size:.2f}x")
        else:
            print(f"Compression ratio: {output_size/input_size:.2f}x")
        
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)


if __name__ == "__main__":
    main() 
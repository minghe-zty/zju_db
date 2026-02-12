# ZJU Database Course Archive

This repository contains coursework and experimental implementations for Zhejiang University's Database course.

## Overview

This is an educational repository featuring a **MiniSQL** database implementation - a complete relational database management system built for learning purposes. The implementation is based on CMU-15445's BusTub framework with significant modifications and extensions.

## Main Project: MiniSQL

The `mini-sql` directory contains a fully-featured relational database engine implementation with the following core modules:

### Core Components

- **Buffer Pool Manager**: LRU and Clock replacement policies for efficient page management
- **Storage Manager**: Disk I/O operations, table heap management, and data page allocation
- **Index Manager**: B+ tree index implementation with iterator support
- **Catalog Manager**: Metadata management for tables and indexes with persistence
- **Record Manager**: Row, Field, Schema, and Column data structures
- **SQL Parser**: Syntax parsing and abstract syntax tree generation
- **Executor**: Query execution engine with operator-based execution plans
- **Transaction Manager**: Concurrency control with lock management and log-based recovery

### Key Features

- Persistent storage with serialization/deserialization
- Extended disk manager with bitmap pages for page allocation tracking
- SQL query parsing and execution
- B+ tree indexing for efficient data retrieval
- Buffer pool management for optimized I/O performance

## Getting Started

For detailed build instructions, development environment setup, and usage guide, please refer to the [MiniSQL README](mini-sql/README.md).

### Quick Build

```bash
cd mini-sql
mkdir build
cd build
cmake ..
make -j
```

### Running Tests

```bash
# Run all tests (from build directory)
./test/minisql_test

# Build and run a specific test (e.g., lru_replacer_test)
make lru_replacer_test
./test/lru_replacer_test
```

## Requirements

- gcc/g++ 8.0+ (Linux) or Apple clang 11.0+ (macOS)
- CMake 3.20+
- GDB 7.0+ (optional)

## Note

⚠️ **Academic Integrity**: Please do not publish your code to any public platform to avoid plagiarism issues.

# Vision and Roadmap: Bitmap Vision Layer

## Executive Summary & Background

Bitmap began as a custom, high-speed visual input pipeline built to feed game frames into a neural network playing *Super Mario World*. To achieve real-time gameplay decisions, it required instant screen capture, fast downsampling, and pixel-level channel manipulation.

Having evolved into a standalone C++ library, **Bitmap's long-term vision is to become a bleeding-edge, zero-dependency image layer for computer vision and image recognition systems**. 

Rather than becoming an all-encompassing tensor or deep learning framework, Bitmap focuses entirely on being the **fastest, cleanest bridge between raw visual inputs (files, memory streams, real-time screen captures) and model-ready tensor buffers**.

---

## Core Vision Statement

> **Bitmap is a high-throughput, zero-dependency C++20 image preprocessing and ingestion library engineered specifically for image recognition pipelines. It transforms raw visual feeds into contiguous, tensor-compatible buffers with sub-millisecond latency and hardware-accelerated SIMD performance.**

---

## Design Principles

1. **Zero External Dependencies**:
   - Built purely on modern C++20 and direct CPU SIMD instructions (AVX2, AVX-512, ARM NEON).
   - No reliance on heavyweight computer vision libraries (OpenCV) or deep learning runtimes (libtorch).
2. **Tensor-Compatible Output Without Tensor Bloat**:
   - Bitmap does not implement a complex N-dimensional Tensor class.
   - Instead, it provides zero-copy and contiguous buffer export mechanisms (`std::span<float>`, `std::span<uint8_t>`) formatted precisely as downstream runtimes expect (planar `CHW`, interleaved `HWC`, normalized, standardized).
3. **Hardware-First Vectorization**:
   - Critical path routines (channel transposing, normalization, spatial interpolation, color conversion) are explicitly vectorized to achieve maximum memory bandwidth utilization.
4. **Robust & Non-Throwing API**:
   - Core operations return explicit value/error types (`Result<T, E>`).
   - Memory is strictly bounds-checked, resilient against malformed inputs, and backed by automated fuzz testing.
5. **Single Cohesive Interface**:
   - Complete migration from legacy dual-API interfaces (`src/bitmap` vs `BmpTool`) into a single, idiomatic, modern C++ API namespace (`BmpTool` or `Bitmap::Vision`).

---

## Target Use Cases

* **Autonomous Vision Agents**: Real-time perception for game-playing AIs, robotics, and desktop automation where latency budgets are under 2–5 ms.
* **Deep Learning Preprocessing Pipelines**: Feeding CNNs, Vision Transformers (ViTs), and object detectors (YOLO, SSD) with preprocessed, letterboxed, and normalized float buffers.
* **Edge & Embedded Recognition**: Environments where OpenCV or Python dependencies are too heavy, fragile, or slow to deploy.

---

## Architectural Blueprint

```
+-------------------------------------------------------------------------------+
|                             Raw Visual Ingestion                              |
|   - BMP File I/O (Disk / Memory Spans)                                        |
|   - Real-Time Windows Capture (GDI / DXGI Desktop Duplication)               |
|   - In-Memory Raw Frame Buffers                                               |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                       Spatial & Vision Transformations                        |
|   - Bilinear & Area-Averaging Resampling (e.g. arbitrary to 224x224, 640x640)  |
|   - Letterboxing & Aspect-Ratio Preserving Canvas Padding                      |
|   - Region of Interest (ROI) & Bounding Box Cropping                          |
|   - 2D Convolutions & Edge Detection (Sobel, Laplacian, Gaussian) via Matrix   |
|   - Color Spaces (Grayscale, YCbCr, HSV)                                      |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                      Tensor-Compatible Output Adapter                         |
|   - Interleaved-to-Planar Transposition (HWC -> CHW)                          |
|   - Vectorized Standard Scaler: (pixel / 255.0 - mean) / std                  |
|   - Contiguous Buffer Export: std::span<float> / std::span<uint8_t>            |
|   - Zero-Copy / Direct Target Memory Writes (In-Place Destination Buffers)    |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                       Downstream Inference Engines                            |
|             (ONNX Runtime, TensorRT, LibTorch, Custom Neural Nets)            |
+-------------------------------------------------------------------------------+
```

---

## Phased Roadmap

### Phase 1: Tensor-Compatible Export & API Unification
* **Goal**: Provide standard model ingestion exports and unify library interfaces.
* **Deliverables**:
  1. **Normalization & Planar Adapter**:
     - `exportPlanarFloat(const Bitmap&, std::span<float> out, NormalizationParams params)`: converts packed pixels into contiguous `CHW` float memory.
     - `exportInterleavedFloat(const Bitmap&, std::span<float> out, NormalizationParams params)`: `HWC` float memory.
     - `exportPlanarUint8(const Bitmap&, std::span<uint8_t> out)`: for quantized/int8 vision models.
     - Support standard pre-sets (e.g., ImageNet mean `[0.485, 0.456, 0.406]`, std `[0.229, 0.224, 0.225]`, or custom ranges `[0.0, 1.0]`, `[-1.0, 1.0]`).
  2. **API Unification**:
     - Route all legacy functions in `src/bitmap` through the modern `include/bitmap.hpp` API.
     - Deprecate old duplicated entry points to maintain a single source of truth.

---

### Phase 2: Vision-Grade Spatial Resampling & Geometry
* **Goal**: High-fidelity geometric operations required by neural network inputs.
* **Deliverables**:
  1. **Bilinear & Area Resampling**:
     - Arbitrary floating-point scaling (replacing integer-only box shrinking).
     - Accurate downsampling to standard CNN input resolutions (`224x224`, `256x256`, `384x384`, `640x640`).
  2. **Letterboxing & Aspect-Ratio Preservation**:
     - Scale-to-fit with automatic padding (configurable padding color, default gray `114/114/114` for YOLO).
  3. **Fast ROI / Sub-Image Slicing**:
     - Zero-copy view / shallow crop of bounding boxes for two-stage detection/classification pipelines.

---

### Phase 3: Mathematical Feature Extraction & Convolutions
* **Goal**: Classical computer vision feature processing powered by `Matrix<T>`.
* **Deliverables**:
  1. **2D Convolution Engine**:
     - Arbitrary kernel convolution on `Matrix<T>`.
  2. **Feature Extraction Kernels**:
     - Sobel horizontal & vertical gradient filters (edge detection).
     - Gaussian blur with configurable sigma and kernel radius.
     - Laplacian edge and high-pass sharpening filters.
  3. **Luma & Grayscale Extraction**:
     - High-speed ITU-R BT.601 / BT.709 luma conversion: $Y = 0.299R + 0.587G + 0.114B$.

---

### Phase 4: SIMD Vectorization & Multi-Threading
* **Goal**: Push preprocessing speeds to the theoretical hardware limit.
* **Deliverables**:
  1. **SIMD Normalization & Transpose**:
     - AVX2 / FMA and ARM NEON paths for simultaneous byte-to-float expansion, de-interleaving (`HWC` $\to$ `CHW`), and `(x - mean) * inv_std`.
  2. **Vectorized Bilinear Interpolation**:
     - SIMD row-buffer processing for image resampling.
  3. **Multi-Threaded Parallel Execution**:
     - Parallel tile processing using C++17/C++20 execution policies (`std::execution::par`) for 4K / high-resolution frames.

---

### Phase 5: Ultra-Low Latency Live Ingestion (The Next-Gen Game AI Pipeline)
* **Goal**: Sub-millisecond frame capture directly from running games and desktop windows.
* **Deliverables**:
  1. **DXGI Desktop Duplication Backend**:
     - Hardware-accelerated GPU desktop/window frame acquisition on Windows (replacing slow GDI `BitBlt`).
     - Capable of sustaining 120–240 FPS frame capture with zero frame drops.
  2. **Thread-Safe Ring Buffer / Double Buffering**:
     - Asynchronous capture thread that continuously stores the latest frame into lock-free shared memory so inference never stalls.

---

## Success Metrics
* **Throughput**: Preprocess (resample 1080p to 224x224, letterbox, normalize, and export CHW float) in **< 1.0 ms** on modern x86/ARM CPUs.
* **Zero Dependencies**: Zero third-party runtime dependencies beyond the C++ standard library and system SDKs.
* **Reliability**: 100% test coverage across all transformations with ongoing fuzzing.

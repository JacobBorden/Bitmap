# Vision and Roadmap: Hardened & Adversarially Robust Image Engine

## Executive Summary

Bitmap began as a real-time visual input pipeline feeding game frames into an autonomous neural network playing *Super Mario World*. 

Moving forward, Bitmap's mission is to become a **lean, low-latency, highly secure, and adversarially hardened image processing library** designed to prepare visual data for custom neural network architectures (such as [Neuro-Net-DLL](https://github.com/JacobBorden/Neuro-Net-DLL)) and recognition systems.

Bitmap is **not** a tensor framework or a generic deep learning library. It is a dedicated **front-line image defense and ingestion engine**: it ingests visual inputs, strictly validates and sanitizes them against exploits, applies filters to strip adversarial perturbations and sensor noise, transforms the image geometry, and outputs clean, flat contiguous memory buffers (`float*` / `std::span<float>` or `uint8_t*`) ready for consumption by custom neural networks.

---

## Core Vision Statement

> **Bitmap is a low-latency, zero-dependency C++20 image processing library engineered for security, stability, and adversarial robustness. It acts as a hardened protective vision layer that immunizes visual pipelines against malformed inputs and adversarial perturbations while delivering deterministic, high-throughput image transformations.**

---

## Strategic Pillars

```
+-------------------------------------------------------------------------------+
|                      PILLAR 1: ZERO-TRUST MEMORY SAFETY                       |
|   - Immune to parser exploits, buffer overflows, and integer wrap-around      |
|   - Continuous fuzz testing (libFuzzer / ASan / UBSan)                        |
|   - Deterministic, non-throwing Result<T, Error> error handling               |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                   PILLAR 2: ADVERSARIAL & NOISE HARDENING                     |
|   - Feature squeezing (bit-depth quantization to kill micro-perturbations)    |
|   - Spatial noise reduction: median filtering, bilateral & total variation   |
|   - Adaptive contrast normalization & high-frequency suppression             |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                    PILLAR 3: LOW-LATENCY TRANSFORMATIONS                      |
|   - Bilinear & area-weighted geometric resizing                               |
|   - Region-of-Interest (ROI) slicing & aspect-ratio letterboxing              |
|   - Hardware SIMD vectorization (AVX2, AVX-512, ARM NEON)                     |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
|                    NEURAL NETWORK INGESTION (FLAT BUFFER)                     |
|   - Contiguous memory buffers (std::span<float>, std::span<uint8_t>)          |
|   - Configurable channel ordering (Planar or Interleaved)                     |
|   - Direct hand-off to custom neural network matrix/tensor inputs             |
+-------------------------------------------------------------------------------+
```

---

### Pillar 1: Zero-Trust Security & Memory Safety
Image parsers are historically among the most targeted attack surfaces for code execution and denial-of-service exploits. Bitmap treats every incoming image as potentially hostile:
* **Arithmetic Overflow Immunity**: Strict verification of all header multiplication arithmetic (e.g., `width * height * bpp`, row padding calculations) preventing integer truncation or wrap-around.
* **Payload Bound Enforcement**: Enforces that pixel data offsets and image sizes never exceed physical buffer lengths.
* **Malicious Parameter Mitigation**: Defensive clamping on scale factors, kernel radii, and dimension sizes to prevent algorithmic complexity attacks or memory exhaustion.
* **Zero Undefined Behavior**: Built and tested with Address Sanitizer (ASan) and Undefined Behavior Sanitizer (UBSan).

---

### Pillar 2: Adversarial Robustness & Noise Suppression
Neural networks are notoriously brittle against adversarial perturbations—imperceptible high-frequency pixel noise or single-pixel shifts deliberately crafted to cause misclassification. Bitmap provides front-line defenses at the image layer:
* **Feature Squeezing / Bit-Depth Quantization**:
  - Quantizing pixel channels (e.g. from 8-bit to 4-bit/5-bit) eliminates subtle gradient-based perturbations (e.g., FGSM, PGD attacks) without degrading coarse semantic features.
* **Spatial Denoising & Median Filtering**:
  - Non-linear median and bilateral filters that remove salt-and-pepper noise and targeted pixel perturbations while preserving structural object edges.
* **Local Contrast Normalization & High-Frequency Suppression**:
  - Mitigates localized lighting attacks and high-frequency noise injection before visual data reaches the neural network.
* **Spatial Jitter & Input Rescaling**:
  - Resampling and randomized scale/pad transforms that break gradient alignment required by white-box adversarial attacks.

---

### Pillar 3: Low Latency & High Throughput
In real-time perception (e.g., autonomous gaming, live screen monitoring, robotic vision):
* **Zero-Allocation Hot Paths**: Image operations reuse caller-provided buffers or pre-allocated scratch pads to eliminate heap allocation jitter.
* **SIMD Vectorization**: Core loops (filtering, color manipulation, quantization, resizing) are optimized with AVX2 and ARM NEON intrinsics.
* **Direct Neural Net Buffer Export**: Exporting directly into simple contiguous memory (`float*` normalized to `[0.0, 1.0]` or `[-1.0, 1.0]`), making ingestion into custom libraries like `Neuro-Net-DLL` completely seamless and zero-copy.

---

## 5-Phase Roadmap

### Phase 1: Security Audit & Zero-Trust Parser Hardening
* **Goal**: Guarantee that the parser and core structures are exploit-proof and memory-safe.
* **Deliverables**:
  1. **Strict Overflow & Boundary Verification**:
     - Comprehensive validation in `load()` and header parsers against integer overflow, negative dimensions, invalid bit depths, and corrupted offsets.
  2. **Security Fuzz Suite Expansion**:
     - Strengthen `tests/fuzz/` with differential fuzzing against malformed BMP structures and invalid memory spans.
  3. **Deterministic Error Handling**:
     - Formalize all failure modes in `BitmapError` (e.g., `DimensionOverflow`, `PayloadTruncated`, `InvalidHeader`).

---

### Phase 2: Adversarial Defense & Image Sanitization Filters
* **Goal**: Implement image-level perturbation defenses to protect neural network inputs.
* **Deliverables**:
  1. **Feature Squeezing (Bit-Depth Reduction)**:
     - Fast channel quantization (reduce 8-bit channels to $N$-bit precision) to remove subtle adversarial noise.
  2. **Median & Spatial Smoothing Filters**:
     - Vectorized median filter ($3 \times 3$, $5 \times 5$) designed to neutralize localized perturbation attacks.
     - Gaussian and bilateral smoothing kernels on `Matrix<T>`.
  3. **Local Contrast & Luma Normalization**:
     - Photometric normalization to resist adversarial lighting/contrast distortions.

---

### Phase 3: Vision Preprocessing & Clean Buffer Export
* **Goal**: Provide lightning-fast geometric preparation and export into neural-network-ready buffers.
* **Deliverables**:
  1. **Bilinear & Area-Averaging Resampling**:
     - Arbitrary float-ratio scaling down to exact model input resolutions (e.g., `64x64`, `128x128`, `224x224`).
  2. **Letterbox Padding & ROI Extraction**:
     - Scale-to-fit with neutral padding to maintain aspect ratio without distortion.
     - Direct bounding box cropping.
  3. **Flat Buffer Export for Custom Neural Networks**:
     - Export to contiguous caller-allocated memory buffers (`float*` or `uint8_t*` in planar or interleaved order with customizable scaling).

---

### Phase 4: SIMD Optimization & Zero-Allocation Pipelines
* **Goal**: Minimize latency on the entire preprocessing and defense pipeline.
* **Deliverables**:
  1. **Vectorized Defense Filters**:
     - AVX2 / ARM NEON acceleration for median filter, bit-depth quantization, and normalization.
  2. **In-Place Pipeline Operations**:
     - Chain filters (e.g., Sanitize $\to$ Quantize $\to$ Resize $\to$ Export) using pre-allocated workspace buffers without intermediate heap churn.

---

### Phase 5: High-Speed Live Frame Ingestion
* **Goal**: Modernize live visual capture for autonomous agents.
* **Deliverables**:
  1. **DXGI Desktop Duplication Backend**:
     - Sub-millisecond GPU-accelerated frame acquisition on Windows (replacing slow GDI `BitBlt`).
  2. **Protected Ring Buffer**:
     - Asynchronous frame capture pipeline ensuring the custom neural net always receives the most recent frame safely.

---

## Success Criteria

1. **Exploit Resilience**: Zero memory corruption, crashes, or undefined behavior across 100,000+ fuzzing iterations on malformed payloads.
2. **Adversarial Noise Mitigation**: Measurable reduction in high-frequency noise and pixel perturbation variance via built-in defense filters.
3. **Latency**: End-to-end ingest, filter, and export of a frame to flat float memory in **< 1.0 ms**.
4. **Zero Dependencies**: Pure, portable C++20 with optional SIMD intrinsics.

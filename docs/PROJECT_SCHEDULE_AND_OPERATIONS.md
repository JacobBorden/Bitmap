# Project Schedule & Daily Operations Plan

This document outlines the **5-week execution schedule** for the Bitmap Hardened Vision Layer. Each week corresponds to one roadmap phase with distinct deliverables, explicit checkpoints, and actionable daily operations.

---

## Master Schedule Overview

| Week | Phase | Focus Area | Weekly Checkpoint |
|:---:|---|---|---|
| **Week 1** | **Phase 1** | Security Audit & Zero-Trust Parser Hardening | Zero integer overflows, strict boundary enforcement, 100K+ fuzz iterations without crashes. |
| **Week 2** | **Phase 2** | Adversarial Defense & Image Sanitization Filters | Bit-depth reduction, median, and bilateral filters verified against perturbation noise. |
| **Week 3** | **Phase 3** | Vision Preprocessing & Flat Buffer Export | Bilinear/area resampling, letterbox padding, and model-ready contiguous buffer exports (`float*`). |
| **Week 4** | **Phase 4** | SIMD Optimization & Zero-Allocation Pipelines | AVX2/NEON vectorization on hot paths, pre-allocated workspaces, sub-millisecond pipeline latency. |
| **Week 5** | **Phase 5** | High-Speed Live Ingestion (DXGI Stream) | GPU DXGI Desktop Duplication capture at 120+ FPS with thread-safe ring buffer. |

---

## Detailed Weekly & Daily Operations

### Week 1: Security Audit & Zero-Trust Parser Hardening
**Goal**: Immunize the library against parser exploits, arithmetic overflows, and malformed image payloads.

* **Day 1: Arithmetic Overflow & Header Validation Audit**
  * Implement safe integer arithmetic utilities (`safe_multiply`, `safe_add`) for all dimensions and byte offsets.
  * Audit `load()` and `src/bitmapfile` to ensure `biWidth * biHeight * (biBitCount / 8)` and row stride calculations `((biWidth * biBitCount + 31) / 32) * 4` cannot wrap around on extreme `uint32_t` values.
  * Enforce maximum sanity bounds on dimensions (e.g., max dimension 65,536 pixels).

* **Day 2: Payload Boundary & Truncation Enforcement**
  * Implement strict physical payload validation: ensure `bfOffBits + biSizeImage <= total_buffer_bytes`.
  * Verify that truncated memory spans reject cleanly with `BitmapError::PayloadTruncated` without out-of-bounds reads.
  * Validate compression formats (strictly enforce uncompressed `BI_RGB = 0` and reject malformed/unsupported compression codes).

* **Day 3: Defensive Clamping & Error Enum Formalization**
  * Expand `BitmapError` enum in `include/bitmap.hpp` to include granular error codes:
    * `DimensionOverflow`
    * `PayloadTruncated`
    * `InvalidColorDepth`
    * `ExceedsMaxDimensions`
  * Add defensive parameter clamping on scaling factors and kernel sizes to eliminate algorithmic complexity / denial-of-service vectors.

* **Day 4: Fuzz Testing Suite Expansion**
  * Add dedicated fuzzers in `tests/fuzz/` targeting:
    * Malformed `BITMAPFILEHEADER` and `BITMAPINFOHEADER` permutations.
    * Memory spans with random byte truncation at every offset.
  * Implement automated corpus seed generation for corrupted BMP variants.

* **Day 5: Week 1 Verification & Checkpoint**
  * **Checkpoint 1 Execution**:
    * Run GoogleTest unit tests with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
    * Execute libFuzzer for 100,000+ iterations across all BMP parsers.
    * Verify zero memory leaks, zero UB, and deterministic error code returns.
    * Code review and milestone tagging on `development`.

---

### Week 2: Adversarial Defense & Image Sanitization Filters
**Goal**: Strip micro-perturbations, gradient attacks, and high-frequency noise before data reaches neural networks.

* **Day 6 (W2D1): Feature Squeezing (Bit-Depth Quantization)**
  * Implement `BmpTool::quantizeChannels(const Bitmap&, uint8_t bitsPerChannel)`:
    * Fast channel depth reduction (e.g., 8-bit down to 4-bit, 5-bit, or 6-bit precision).
    * Maps pixel values to coarse buckets, destroying low-amplitude adversarial gradients (FGSM/PGD).
  * Add unit tests verifying bucket mapping accuracy and channel preservation.

* **Day 7 (W2D2): Non-Linear Median Filtering ($3 \times 3$ and $5 \times 5$)**
  * Implement non-linear median filter on `Matrix<Pixel>` and `BmpTool::Bitmap`.
  * Optimize $3 \times 3$ median computation using sorting networks (no dynamic memory allocation).
  * Verify removal of single-pixel adversarial attacks and impulse salt-and-pepper noise while preserving edges.

* **Day 8 (W2D3): Spatial Smoothing (Gaussian & Bilateral Kernels)**
  * Implement 2D Gaussian blur filter with configurable standard deviation ($\sigma$) and kernel radius.
  * Implement edge-preserving bilateral filtering to smooth out subtle spatial noise without blurring critical classification contours.

* **Day 9 (W2D4): Local Contrast & Photometric Normalization**
  * Implement local contrast normalization: suppresses localized glare, adversarial lighting shifts, and high-frequency brightness spikes.
  * Implement fast ITU-R BT.601 / BT.709 photometric luma extraction for single-channel grayscale model ingestion.

* **Day 10 (W2D5): Week 2 Verification & Checkpoint**
  * **Checkpoint 2 Execution**:
    * Construct synthetic adversarial test patterns (high-frequency noise injection, single-pixel attacks).
    * Run test suite measuring perturbation reduction ratio (noise variance before vs after filters).
    * Ensure all defense filters handle image borders and edges safely.
    * Commit and tag Week 2 milestone on `development`.

---

### Week 3: Vision Preprocessing & Flat Buffer Export
**Goal**: Deliver precise geometric transformations and direct contiguous memory exports for custom neural nets.

* **Day 11 (W3D1): Bilinear Interpolation Resampling**
  * Implement `BmpTool::resizeBilinear(const Bitmap&, uint32_t targetWidth, uint32_t targetHeight)`:
    * Continuous floating-point ratio coordinate mapping with 4-neighbor linear interpolation.
    * Enables clean resizing to standard model resolutions (`64x64`, `128x128`, `224x224`, `640x640`).

* **Day 12 (W3D2): Area-Averaging Resampling**
  * Implement area-weighted box sampling for large downscaling factors (e.g. 4K down to 224x224) to eliminate Moiré and aliasing artifacts.

* **Day 13 (W3D3): Letterbox Padding & Aspect-Ratio Preservation**
  * Implement `BmpTool::letterbox(const Bitmap&, uint32_t targetWidth, uint32_t targetHeight, Pixel padColor)`:
    * Scales image to fit bounding box while maintaining exact aspect ratio.
    * Centers image and applies uniform padding (defaulting to neutral gray `114/114/114` as required by object detection models like YOLO).

* **Day 14 (W3D4): Region of Interest (ROI) Crop & Flat Buffer Export**
  * Implement bounding box / ROI slicing: `BmpTool::crop(const Bitmap&, uint32_t x, uint32_t y, uint32_t w, uint32_t h)`.
  * Implement flat contiguous buffer exports:
    * `BmpTool::exportPlanarFloat(const Bitmap&, std::span<float> outBuffer, NormalizationParams norm)` ($[C, H, W]$ layout).
    * `BmpTool::exportInterleavedFloat(const Bitmap&, std::span<float> outBuffer, NormalizationParams norm)` ($[H, W, C]$ layout).
    * `BmpTool::exportPlanarUint8(const Bitmap&, std::span<uint8_t> outBuffer)`.

* **Day 15 (W3D5): Week 3 Verification & Checkpoint**
  * **Checkpoint 3 Execution**:
    * Test export buffers directly against expected neural network input specifications.
    * Validate scaling precision across float and uint8 outputs with zero memory leaks.
    * Validate letterboxing aspect ratio math across extreme aspect ratios (panoramic, vertical).
    * Commit and tag Week 3 milestone on `development`.

---

### Week 4: SIMD Optimization & Zero-Allocation Pipelines
**Goal**: Maximize throughput, minimize latency, and eliminate heap allocation jitter.

* **Day 16 (W4D1): SIMD Bit-Depth Quantization & Normalization**
  * Implement AVX2 and ARM NEON vectorized kernels for bit-depth quantization (feature squeezing).
  * Implement vectorized byte-to-float normalization `(x - mean) * inv_std` processing 8/16 pixels per instruction using Fused Multiply-Add (FMA).

* **Day 17 (W4D2): Vectorized Bilinear Resampling**
  * Optimize bilinear interpolation horizontal and vertical passes with SIMD row buffering.
  * Benchmark throughput scaling on AVX2 vs scalar reference.

* **Day 18 (W4D3): Vectorized Sorting Networks for Median Filtering**
  * Vectorize $3 \times 3$ median filter using SIMD min/max register swizzles (branchless sorting networks).

* **Day 19 (W4D4): Zero-Allocation Pipeline Architecture**
  * Implement `BmpTool::Pipeline` with pre-allocated scratch workspace:
    * Allows chaining: `Ingest -> Sanitize -> Denoise -> Resize -> Export` without any intermediate `malloc`/`free` calls during runtime loops.
  * Guarantee zero heap allocations during live inference loops.

* **Day 20 (W4D5): Week 4 Verification & Checkpoint**
  * **Checkpoint 4 Execution**:
    * Latency profiling on Release builds: benchmark full 1080p pipeline execution time (target < 1.0 ms).
    * Verify bit-for-bit numerical equivalence between SIMD paths and scalar reference paths.
    * Memory profiler check confirming zero allocations in hot processing loop.
    * Commit and tag Week 4 milestone on `development`.

---

### Week 5: High-Speed Live Ingestion (DXGI Stream)
**Goal**: Deliver low-latency, real-time screen capture for autonomous game agents and live vision.

* **Day 21 (W5D1): Windows Desktop Duplication (DXGI) Backend**
  * Implement `DxgiCaptureSession` on Windows using DirectX 11 Desktop Duplication API.
  * Acquire GPU desktop/window frame pointers directly, replacing legacy GDI `BitBlt`.

* **Day 22 (W5D2): GPU-to-CPU Staging & Swizzling**
  * Implement high-speed mapped staging texture copy to CPU memory.
  * Connect DXGI frame output directly to SIMD BGRA-to-RGBA swizzlers.

* **Day 23 (W5D3): Thread-Safe Asynchronous Ring Buffer**
  * Implement a lock-free / double-buffered circular frame buffer:
    * Background thread continuously acquires screen frames at display refresh rate (60–144+ FPS).
    * Main neural network inference thread fetches the latest complete frame with zero lock contention.

* **Day 24 (W5D4): End-to-End Live Agent Integration**
  * Integrate the complete pipeline:
    `Live Screen Capture (DXGI) -> Noise Filter -> Resize -> Letterbox -> Export Flat Float Buffer`.
  * Test continuous live capture loop running over 10,000 frames without frame drops.

* **Day 25 (W5D5): Final Milestone Verification & Project Review**
  * **Checkpoint 5 (Final Release) Execution**:
    * Full regression test pass (all 67+ unit tests + new tests green).
    * Continuous fuzzing verification pass.
    * Performance validation: capture + preprocess sustained at 120+ FPS.
    * Merge `development` into `master` and tag release `v2.0.0`.

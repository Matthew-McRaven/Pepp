/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#pragma once

#ifdef __GNUG__
#ifndef LIKELY
#define LIKELY(x) __builtin_expect((x), 1)
#endif
#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect((x), 0)
#endif
#ifndef PEPP_COLD_PATH
#define PEPP_COLD_PATH() __attribute__((cold))
#endif
#ifndef PEPP_HOT_PATH
#define PEPP_HOT_PATH() __attribute__((hot))
#endif
#define PEPP_ALWAYS_INLINE __attribute__((always_inline))
#define PEPP_NOINLINE __attribute__((noinline))
#define PEPP_UNREACHABLE() __builtin_unreachable()
#define RISCV_EXPORT __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define PEPP_COLD_PATH() /* */
#define PEPP_HOT_PATH()  /* */
#define PEPP_ALWAYS_INLINE __forceinline
#define PEPP_NOINLINE __declspec(noinline)
#define PEPP_UNREACHABLE() __assume(0)
#define RISCV_EXPORT __declspec(dllexport)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define PEPP_COLD_PATH()   /* */
#define PEPP_HOT_PATH()    /* */
#define PEPP_ALWAYS_INLINE /* */
#define PEPP_NOINLINE      /* */
#define PEPP_UNREACHABLE() /* */
#define RISCV_EXPORT       /* */
#endif

#ifdef __HAVE_BUILTIN_SPECULATION_SAFE_VALUE
#define RISCV_SPECSAFE(x) __builtin_speculation_safe_value(x)
#else
#define RISCV_SPECSAFE(x) (x)
#endif

#ifndef RISCV_INTERNAL
#if defined(__GNUG__) && !defined(_WIN32)
#define RISCV_INTERNAL __attribute__((visibility("internal")))
#else
#define RISCV_INTERNAL /* */
#endif
#endif

#ifdef __APPLE__
#include "TargetConditionals.h" // TARGET_* macros
#endif

#ifndef ANTI_FINGERPRINTING_MASK_MICROS
#define ANTI_FINGERPRINTING_MASK_MICROS() ~0x3LL
#endif
#ifndef ANTI_FINGERPRINTING_MASK_NANOS
#define ANTI_FINGERPRINTING_MASK_NANOS() ~0x3FFFLL
#endif

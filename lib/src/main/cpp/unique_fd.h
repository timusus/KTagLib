/*
 * Copyright (C) 2018 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <android/fdsan.h>
#include <unistd.h>

#include <utility>

struct unique_fd {
    unique_fd() = default;

    explicit unique_fd(int fd) {
        reset(fd);
    }

    unique_fd(const unique_fd &copy) = delete;

    unique_fd(unique_fd &&move) {
        *this = std::move(move);
    }

    ~unique_fd() {
        reset();
    }

    unique_fd &operator=(const unique_fd &copy) = delete;

    unique_fd &operator=(unique_fd &&move) {
        if (this == &move) {
            return *this;
        }

        reset();

        if (move.fd_ != -1) {
            fd_ = move.fd_;
            move.fd_ = -1;
        }

        return *this;
    }

    int get() { return fd_; }

    int release() {
        if (fd_ == -1) {
            return -1;
        }

        int fd = fd_;
        fd_ = -1;

        return fd;
    }

    void reset(int new_fd = -1) {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }

        if (new_fd != -1) {
            fd_ = new_fd;
        }
    }

private:
    int fd_ = -1;

    // The obvious choice of tag to use is the address of the object.
    uint64_t tag() {
        return reinterpret_cast<uint64_t>(this);
    }
};
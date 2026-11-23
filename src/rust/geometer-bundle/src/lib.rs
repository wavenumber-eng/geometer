//! Shared admission and extraction of an explicitly selected native Geometer ZIP.
//!
//! The host owns acquisition, its release pin, cache-root selection and override
//! policy. This crate never downloads or starts executables. Materialization
//! verifies the archive and uses crash-released OS locking for its exact digest;
//! callers may memoize success but should retry recoverable failures later.

pub mod archive;
mod cache;

pub use cache::{Bundle, materialize};

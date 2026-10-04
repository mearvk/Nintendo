package com.mearvk.nintendo.strategy;

/** A format, range, or analysis error in the NES strategy-analysis module. */
public class NesException extends Exception {
    public NesException(String message) { super(message); }
    public NesException(String message, Throwable cause) { super(message, cause); }
}

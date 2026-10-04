package com.mearvk.nintendo.editor;

/** A format, range, encoding, or length-rule violation in the NES editor. */
public class NesException extends Exception {
    public NesException(String message) { super(message); }
    public NesException(String message, Throwable cause) { super(message, cause); }
}

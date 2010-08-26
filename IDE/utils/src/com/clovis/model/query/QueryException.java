package com.clovis.model.query;
/**
 * This is the query interface for database. Any database implementation need 
 * to implement this interface for querying it.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public class QueryException extends Exception 
{
    /**
     * exception with null as its detail message.
     */
    public QueryException()                             { super(); }
    /**
     * @param message the detail message.
     * exception with the specified detail message.
     */
    public QueryException(String message)               { super(message); }
    /**
     * exception with the specified detail message and cause.
     * @param message the detail message.
     * @param t The cause.
     */
    public QueryException(String message, Throwable t)  { super(message, t); }
    /**
     * exception with the specified cause and a detail message.
     * @param t The cause.
     */
    public QueryException(Throwable t)                  { super(t); }
}

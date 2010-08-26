package com.clovis.model.query;
/**
 * This is the query interface for database. Any database implementation need 
 * to implement this interface for querying it.
 * @author <a href="nadeem@clovissolutions.com">Nadeem</a>
 */
public interface IQuery
{
    /**
     * Store the xml in the database.
     * @param data XML.
     */
    void   set(String data) throws QueryException;
    /**
     * Retrieve the data.
     * @param name      Name of complex Element.
     * @param condition SQL Condition.
     * @param depth     Depth of tree.
     */
    String get(String name, String condition, int depth) throws QueryException;
}

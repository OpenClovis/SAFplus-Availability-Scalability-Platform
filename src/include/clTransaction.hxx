/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.
 */

#ifndef CLMGTTRANSACTION_HXX_
#define CLMGTTRANSACTION_HXX_

#include <vector>
#include <string>
#include <exception>
#include "clHandleApi.hxx"

namespace SAFplus
{
  class TransactionOperation;
  class Transaction;
  
  class TransactionException: public std::exception
  {
  protected:
    Transaction& txn;
  public:
    TransactionException(Transaction& t):txn(t) {}
    virtual const char* what() const throw()
    {
      return "TODO";
    }
  };
  
  class Transaction
  {
  protected:
      Handle mHandle;
      std::vector<TransactionOperation*> mOperations;
  public:
      Transaction();
      virtual ~Transaction();

      void addOperation(TransactionOperation *operation);

      void commit();
      void abort();
  };

  class TransactionOperation
  {
  public:
      virtual ~TransactionOperation() {};
      virtual bool validate(Transaction& t) = 0;
      virtual void commit() = 0;
      virtual void abort() = 0;
  };

  template <class T> class SimpleTxnOperation: public TransactionOperation
    {
  public:
    virtual ~SimpleTxnOperation() {};
    SimpleTxnOperation(T* destination, const T& val):dest(destination),value(val) {}
    T* dest;
    T value;
    virtual bool validate(Transaction& t) { return true; }
    virtual void commit() { *dest = value; }
    virtual void abort() {};
    };

  extern Transaction NO_TXN;
};

#endif /* CLMGTTRANSACTION_HXX_ */

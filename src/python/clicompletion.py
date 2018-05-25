#!/usr/bin/env python
import socket
from socket import error as socket_error
import sys, getopt
from   select import poll, POLLIN, POLLOUT, POLLHUP
import select
import termios
import struct, fcntl, os
import errno
import time

messages = [ 'This is the message. ',
             'It will be sent ',
             'in parts.',
             ]
# index : (0 : fd),(1, infd) , (2 , outfd) 
# [file descriptor, event] tuple

GT_SIGN = ">"
SPACE	= " "

block0 = ""
block0_remain = 0
block1 = ""
block1_remain = 0
in0buf = ""
in1buf = ""
out0buf = ""
out1buf = ""
nl_interactive = "\r\n"
nl_batch       = "\n"
nl = ""
nl_len = 1
old_raw = 0
fd = 0
infd = sys.stdin.fileno()
outfd = sys.stdin.fileno()
array_poll = [[fd,0],[infd,0],[outfd,0]] 

def write_fill_server_noesc(fd , buf, length):
	condition = True
	done = 0
	count = 0
	while condition :
		count = os.write(fd, buf[done:(length-done)])
		if count < 0:
			if errno == errno.EWOULDBLOCK | errno == errno.EAGAIN:
				block0 = buf[done:]
				block0_remain = buff[(length-done):]
				# wait until ConfD is ready for more data
				array_poll[0][1] |= termios.POLLOUT
				# block user input
				array_poll[1][1] &= ~(termios.POLLIN)
				return 0
				if errno != EINTR :
					return count
			count = 0
		done = done + count
		if done >= length: break
	return length
	
def write_fill_server(fd , buf, length):
	global out0buf
	i = n = ret = 0
	for i in range(length):
		if buf[i] == 0:
			#out0buf[n] = 0
			out0buf = out0buf[:n] + 0 + out0buf[(n+1):]
			n += 1
		#temp = buf[i]
		out0buf = out0buf[:n] + buf[i] + out0buf[(n+1):]
		n += 1
	ret = write_fill_server_noesc(fd, out0buf, n)
	return ret

def write_fill_term(fd , buf, length):
	condition = True
	done = 0
	count = 0
	while condition :
		count = os.write(fd, buf[done:(length-done)])
		if count < 0:
			if errno == errno.EWOULDBLOCK | errno == errno.EAGAIN:
				block0 = buf[done:]
				block0_remain = buff[(length-done):]
				# wait until term is ready for more data
				array_poll[2][1] |= termios.POLLOUT
				# block additional input from server
				array_poll[0][1] &= ~(termios.POLLIN)
				return 0
				if errno != EINTR :
					return count
			count = 0
		done = done + count
		if done >= length: break
	return length

#raw mode
old = termios.tcgetattr(0)             
def tty_raw (fd):
	global old
	old = termios.tcgetattr(fd)   
	new = termios.tcgetattr(fd)
	
	iflag = old[0]
	oflag = old[1]
	cflag = old[2]
	lflag = old[3]
	
	iflag = iflag & ~(termios.ISTRIP | termios.IXON | termios.BRKINT | termios.ICRNL)
	lflag = lflag & ~(termios.ECHO|termios.ICANON|termios.IEXTEN|termios.ISIG)
	cflag = (cflag & ~(termios.CSIZE | termios.PARENB)) | termios.CS8
	oflag = oflag | termios.OPOST | termios.ONLCR
	
	new[0] = iflag
	new[1] = oflag
	new[2] = cflag
	new[3] = lflag
	
	new[6][termios.VMIN] = 1
	new[6][termios.VTIME] = 0
	
	termios.tcsetattr(fd, termios.TCSAFLUSH, new)
	return 0

def tty_restore(fd):
	global old
	if termios.tcsetattr(fd, termios.TCSAFLUSH, old) < 0 :
		return -1
	return 0;


#set nonblocking
prev_fd_flags = prev_infd_flags = prev_outfd_flags = 0
def set_nonblocking(fd, outfd):
	prev_fd_flags = fcntl.fcntl(fd, fcntl.F_GETFL, 0)
	fcntl.fcntl(fd, fcntl.F_SETFL, prev_fd_flags | os.O_NONBLOCK)
	
	prev_outfd_flags = fcntl.fcntl(outfd, fcntl.F_GETFL, 0)
	fcntl.fcntl(outfd, fcntl.F_SETFL, prev_outfd_flags | os.O_NONBLOCK)
	
	#prev_infd_flags = fcntl.fcntl(infd, fcntl.F_GETFL, 0)
	#fcntl.fcntl(infd, fcntl.F_SETFL, prev_infd_flags | os.O_NONBLOCK)

def main(resolver, addr):
	block0 = ""
	block0_remain = 0
	block1 = ""
	block1_remain = 0
	in0buf = ""
	in1buf = ""
	out0buf = ""
	out1buf = ""
	nl_interactive = "\r\n"
	nl_batch       = "\n"
	nl = ""
	nl_len = 1
	old_raw = 0
	fd = 0
	infd = sys.stdin.fileno()
	outfd = sys.stdin.fileno()
	array_poll = [[fd,0],[infd,0],[outfd,0]]
	  
	server_address = (addr, 12345)

	# Create a TCP/IP socket
	#socks = [ socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	#          socket.socket(socket.AF_INET, socket.SOCK_STREAM),
	#          ]
	socks =  socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# Connect the socket to the port where the server is listening
	#print >>sys.stderr, 'connecting to %s port %s' % server_address
	#for s in socks:
	#    s.connect(server_address)
	socks.connect(server_address)
	fd = socks.fileno()
	array_poll[0][0] = fd
	infd = sys.stdin.fileno()
	outfd = sys.stdout.fileno()

	escape_count=0
	escape=-1
	exit_code = 4
	next_may_be_exit_code = 0

	interactive = os.isatty(infd);

	if interactive:
		tty_raw(infd)
		if old_raw:
			nl = nl_interactive
			nl_len = 2
		else:
			#print "not old raw"
			nl = nl_batch
			nl_len = 1
	else:
		nl = nl_batch
		nl_len = 1
			
	poller = select.poll()
	poller.register(infd, POLLIN)
	poller.register(fd, POLLIN)
	poller.register(outfd,0)

	set_nonblocking(fd, outfd)
	#cmd="connect a root clovis localhost"
	#argList.execute(cmd,argList)
	i = 0
	cmd = ""
	while 1:
		fd_event_list = poller.poll(-1)
		
		#print "size fd_event_list = %d \n"  %len(fd_event_list)
		for iterfd, event in fd_event_list:
			#TODO : process fd and event
			#print "event : %d" %(event)
			if iterfd == fd:
				array_poll[0][0] = iterfd
				array_poll[0][1] = event
			elif iterfd == infd:
				array_poll[1][0] = iterfd
				array_poll[1][1] = event
			elif iterfd == outfd:
				array_poll[2][0] = iterfd
				array_poll[2][1] = event
				
			
			#check if we are waiting for server to be ready to accept more data
		if array_poll[0][1] & POLLOUT:
			#print "iterfd is socket fd, event is POLLOUT"
			array_poll[0][1] &= ~POLLOUT
			array_poll[1][1] |= POLLIN
			
			write_fill_server_noesc(fd, block0, block0_remain)
			
				
			#if (fds[2].revents & POLLOUT && fds[2].events & POLLOUT) {
			#check if we are waiting for terminal to be ready to accept more data
		if array_poll[2][1] & POLLOUT :
			#print "iterfd is stdout"
			array_poll[2][1] &= ~POLLOUT
			array_poll[0][1] |= POLLIN
			
			n = write_fill_term(outfd, block1, block1_remain)
			
			if n < 0:
				break
		
			# Data from terminal side?
		if ((array_poll[1][1] & (POLLIN | POLLHUP)) and ( array_poll[1][1] & POLLIN)):
			#print "iterfd is infd, Data from terminal side"
			lenbuf = 0
			#print "in0buf = "+ in0buf
			try:
				in0buf = os.read(array_poll[1][0],1024)
				
				lenbuf = len(in0buf)
				#print "lenbuf = %d" %lenbuf
			except socket_error as sock_error:
				### do later======================
				if(lenbuf < 0 and sock_error.errno != errno.ECONNRESET):
					break
				elif (lenbuf == 0 or (lenbuf < 0 and sock_error.errno == errno.ECONNRESET) ):
					if interactive:
						break
					else:
						socks.shutdown(socket.SHUT_WR)
						
				#print "in0buf = "+ in0buf
			if lenbuf == 0:
				if interactive:
					break
				else:
					socks.shutdown(socket.SHUT_WR)
			
			###do later=====================
			#Scan for global panic character: three consecutive
			#   ctrl-_
			for i in range(lenbuf):
				if in0buf[i] == 31:
					escape_count += 1
					if escape_count == 3:
						break
				elif escape != -1 and in0buf[i] == escape:
					break
				else:
					escape_count = 0
			
			if lenbuf > 0:
				ret = write_fill_server(fd, in0buf, lenbuf)
				if (ret < 0 ):
					break
		elif array_poll[1][1] & ~POLLOUT:
			if interactive:
				break
			else:
				socks.shutdown(socket.SHUT_WR)
		
			#Data from server side 
		if (array_poll[0][1] & POLLIN) :
			#print "wait data from server side, event is POLLIN"
			length_in1buf = 0
			try:
				in1buf = os.read(fd, 1024)
				length_in1buf = len(in1buf)
			except socket_error as sock_error:
				tty_restore(1)
				if(length_in1buf < 0 and socket_error.errno != errno.ECONNRESET):
					break
				elif (length_in1buf == 0 or (length_in1buf < 0 and socket_error.errno == errno.ECONNRESET) ):
					if interactive:
						break
					else:
						socks.shutdown(socket.SHUT_WR)
			
			if length_in1buf == 0:
				if interactive:
					break
				else:
					socks.shutdown(socket.SHUT_WR)
			
			#escape \n with \r\n
			j = 0
			#print "in1buf =%s&& len = %d\n" %(in1buf,len(in1buf))
			if in1buf[length_in1buf-1] == '\n':
				curdircmd = in1buf[:(length_in1buf-1)]
				
				pos = curdircmd.find(GT_SIGN)
				tmpcurdir = curdircmd[:pos]

				cmd = curdircmd[pos+2:]

				
				if cmd == "exit\n" or cmd == "exit":
					time.sleep(2)
					tty_restore(1)
				resolver.execute(cmd,resolver)

				sys.stdout.write(resolver.curdir)
				sys.stdout.write(GT_SIGN)
				sys.stdout.write(SPACE)
				sys.stdout.flush()
			else:
				for i in range(length_in1buf):
					if next_may_be_exit_code:
						next_may_be_exit_code = 0
						if in1buf[i] != '\0':
							exit_code = in1buf[i]
							if exit_code ==  254:
								exit_code=0
							j-=1;
						else:
							#out1buf[j]=in1buf[i]
							out1buf= out1buf[:j] + in1buf[i] + out1buf[(j+1):]
					elif in1buf[i] == '\n' and nl_len == 2:
						#out1buf[j] = nl[0]
						out1buf= out1buf[:j] + nl[0] + out1buf[(j+1):]
						j+=1
						#out1buf[j] = nl[1]
						out1buf= out1buf[:j] + nl[1] + out1buf[(j+1):]
					elif in1buf[i] == '\0':
						next_may_be_exit_code = 1
						j -= 1
					else:
						#out1buf[j] = in1buf[i]
						out1buf= out1buf[:j] + in1buf[i] + out1buf[(j+1):]
					j += 1
						
				write_fill_term(array_poll[2][0], out1buf, length_in1buf)	
		elif (array_poll[0][1] & ~POLLOUT) and (array_poll[2][1] & ~POLLOUT):
			print "wait data from server side, else"
			#exit error
			break
		array_poll = [[0,0],[0,0],[0,0]]
	#for message in messages:
		
		# Send messages on both sockets
		#for s in socks:
	#        print >>sys.stderr, '%s: sending "%s"' % (socks.getsockname(), message)
	#        socks.send(message)

		# Read responses on both sockets
		#for s in socks:
	#        data = socks.recv(1024)
	#        print >>sys.stderr, '%s: received "%s"' % (socks.getsockname(), data)
	#        if not data:
	#            print >>sys.stderr, 'closing socket', socks.getsockname()
	#            socks.close()

	#def main():
		
	if interactive:
		#print "tty_restore"
		tty_restore(1)

if __name__ == '__main__':
    main(sys.argv)


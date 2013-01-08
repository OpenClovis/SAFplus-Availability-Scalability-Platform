# -*- coding: utf-8 -*-

"""Kid-savvy HTTP Server.

Written by Christoph Zwerschke based on CGIHTTPServer 0.4.

This module builds on SimpleHTTPServer by implementing GET and POST
requests to Kid templates.

In all cases, the implementation is intentionally naive -- all
requests are executed by the same process and sychronously.

Code to create and run the server looks like this:

    from kid.server import HTTPServer
    host, port = 'localhost', 8000
    HTTPServer((host, port)).serve_forever()

This serves files and kid templates from the current directory
and any of its subdirectories.

If you want the server to be accessible via the network,
use your local host name or an empty string as the host.
(Security warning: Don't do this unless you are inside a firewall.)

You can also call the test() function to run the server, or run this
module as a script, providing host and port as command line arguments.

The Kid templates have access to the following predefined objects:

    FieldStorage (access to GET/POST variables)
    environ (CGI environment)
    request (the request handler object)

Here is a simple Kid template you can use to test the server:

    <html xmlns="http://www.w3.org/1999/xhtml"
    xmlns:py="http://purl.org/kid/ns#">
    <head><title>Python Expression Evaluator</title></head>
    <body>
    <h3 py:if="FieldStorage.has_key('expr')">
    ${FieldStorage.getvalue('expr')} =
    ${eval(FieldStorage.getvalue('expr'))}</h3>
    <form action="${environ['SCRIPT_NAME']}" method="post">
    <h3>Enter a Python expression:</h3>
    <input name="expr" type="text" size="40" maxlength="40" />
    <input type="submit" value="Submit" />
    </form>
    </body>
    </html>
"""

__revision__ = "$Rev: 492 $"
__date__ = "$Date: 2007-07-06 21:38:45 -0400 (Fri, 06 Jul 2007) $"
__author__ = "Christoph Zwerschke (cito@online.de)"
__copyright__ = "Copyright 2005, Christoph Zwerschke"
__license__ = "MIT <http://www.opensource.org/licenses/mit-license.php>"

import os.path
from urllib import unquote
from BaseHTTPServer import HTTPServer as BaseHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from cgi import FieldStorage

from kid import load_template

__all__ = ["HTTPServer", "HTTPRequestHandler"]


default_host = 'localhost'
default_port = 8000


class HTTPRequestHandler(SimpleHTTPRequestHandler):

    """Complete HTTP server with GET, HEAD and POST commands.
    GET and HEAD also support running Kid templates.
    The POST command is *only* implemented for Kid templates."""

    def do_POST(self):
        """Serve a POST request implemented for Kid templates."""
        if self.is_kid():
            self.run_kid()
        else:
            self.send_error(501, "Can only POST to Kid templates")

    def send_head(self):
        """Version of send_head that supports Kid templates."""
        if self.is_kid():
            return self.run_kid()
        else:
            return SimpleHTTPRequestHandler.send_head(self)

    def is_kid(self):
        """Test whether self.path corresponds to a Kid template.

        The default implementation tests whether the path ends
        with one of the strings in the list self.kid_extensions.

        """
        path = self.path
        i = path.rfind('?')
        if i >= 0:
            path, query = path[:i], path[i+1:]
        else:
            query = ''
        for x in self.kid_extensions:
            if path.endswith(x):
                self.cgi_info = path, query
                return True
        return False

    kid_extensions = ['.kid', '.kid.html']

    def run_kid(self):
        """Execute a Kid template."""
        scriptname, query = self.cgi_info
        scriptfile = self.translate_path(scriptname)
        if not os.path.exists(scriptfile):
            self.send_error(404, "No such Kid template (%r)"
                % scriptname)
            return
        if not os.path.isfile(scriptfile):
            self.send_error(403, "Kid template is not a plain file (%r)"
                % scriptname)
            return

        env = {}
        env['SERVER_SOFTWARE'] = self.version_string()
        env['SERVER_NAME'] = self.server.server_name
        env['GATEWAY_INTERFACE'] = 'CGI/1.1'
        env['SERVER_PROTOCOL'] = self.protocol_version
        env['SERVER_PORT'] = str(self.server.server_port)
        env['REQUEST_METHOD'] = self.command
        uqpath = unquote(scriptname)
        env['PATH_INFO'] = uqpath
        env['PATH_TRANSLATED'] = self.translate_path(uqpath)
        env['SCRIPT_NAME'] = scriptname
        if query:
            env['QUERY_STRING'] = query
        host = self.address_string()
        if host != self.client_address[0]:
            env['REMOTE_HOST'] = host
        env['REMOTE_ADDR'] = self.client_address[0]
        authorization = self.headers.getheader("authorization")
        if authorization:
            authorization = authorization.split()
            if len(authorization) == 2:
                import base64, binascii
                env['AUTH_TYPE'] = authorization[0]
                if authorization[0].lower() == "basic":
                    try:
                        authorization = base64.decodestring(authorization[1])
                    except binascii.Error:
                        pass
                    else:
                        authorization = authorization.split(':')
                        if len(authorization) == 2:
                            env['REMOTE_USER'] = authorization[0]
        if self.headers.typeheader is None:
            env['CONTENT_TYPE'] = self.headers.type
        else:
            env['CONTENT_TYPE'] = self.headers.typeheader
        length = self.headers.getheader('content-length')
        if length:
            env['CONTENT_LENGTH'] = length
        accept = []
        for line in self.headers.getallmatchingheaders('accept'):
            if line[:1] in "\t\n\r ":
                accept.append(line.strip())
            else:
                accept = accept + line[7:].split(',')
        env['HTTP_ACCEPT'] = ','.join(accept)
        ua = self.headers.getheader('user-agent')
        if ua:
            env['HTTP_USER_AGENT'] = ua
        co = filter(None, self.headers.getheaders('cookie'))
        if co:
            env['HTTP_COOKIE'] = ', '.join(co)

        self.send_response(200, "Script output follows")

        # Execute template in this process
        try:
            template_module = load_template(scriptfile, cache=True)
            template = template_module.Template(
                request=self, environ=env,
                FieldStorage=FieldStorage(self.rfile, environ=env))
            s = str(template)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-Length", str(len(s)))
            self.end_headers()
            self.wfile.write(s)
        except Exception, e:
            self.log_error("Kid template exception: %s", str(e))
        else:
            self.log_message("Kid template exited OK")


class HTTPServer(BaseHTTPServer):

    def __init__(self,
        server_address=None,
        RequestHandlerClass=HTTPRequestHandler):
        if server_address is None:
            server_address = (default_host, default_port)
        BaseHTTPServer.__init__(self,
            server_address, HTTPRequestHandler)


def test(server_address=None,
            HandlerClass=HTTPRequestHandler,
            ServerClass=HTTPServer,
            protocol="HTTP/1.0"):
    """Test the HTTP request handler class."""

    HandlerClass.protocol_version = protocol
    server = ServerClass(server_address, HandlerClass)
    sa = server.socket.getsockname()
    print "Serving HTTP on", sa[0], "port", sa[1], "..."
    server.serve_forever()


def main():
    """This runs the Kid-savvy HTTP server.

    Provide host and port as command line arguments.
    The current directory serves as the root directory.

    """

    from sys import argv, exit

    if len(argv) > 3:
        print "Usage:", argv[0], "[host]:[port]"
        exit(2)

    if len(argv) < 2:
        server_address = (default_host, default_port)
    else:
        if len(argv) == 3:
            host = argv[1]
            port = argv[2]
        else:
            host = argv[1].split(':', 1)
            if len(host) < 2:
                host = host[0]
                if host.isdigit():
                    port = host
                    host = ''
                else:
                    port = None
            else:
                host, port = host
        if port:
            if port.isdigit():
                port = int(port)
            else:
                print "Bad port number."
                exit(1)
        else:
            port = default_port
        server_address = (host, port)

    test(server_address)


if __name__ == '__main__':
    main()

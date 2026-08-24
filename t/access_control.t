#!/usr/bin/perl

# Tests for ngx_http_access_control_module.

###############################################################################

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use Test::Nginx;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->has(qw/http rewrite ngx_condition_module
	ngx_http_access_control_module/)->plan(12);

$t->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        condition blocked str_eq $arg_block yes;
        condition teapot str_eq $arg_status teapot;

        location = /basic {
            access allow $arg_allow;
            access deny $arg_deny;
            alias %%TESTDIR%%/ok;
        }

        location = /when {
            when blocked {
                access deny;
            }
            access allow;
            alias %%TESTDIR%%/ok;
        }

        location = /status {
            when teapot {
                access_deny_status 418;
            }
            access_deny_status 401;
            access deny;
            alias %%TESTDIR%%/ok;
        }

        location /parent/ {
            access deny $arg_parent;

            location = /parent/inherit {
                alias %%TESTDIR%%/ok;
            }

            location = /parent/on {
                access allow;
                alias %%TESTDIR%%/ok;
            }

            location = /parent/before {
                access_inherit before;
                access allow;
                alias %%TESTDIR%%/ok;
            }

            location = /parent/after {
                access_inherit after;
                access allow;
                alias %%TESTDIR%%/ok;
            }

            location = /parent/off {
                access_inherit off;
                access deny $arg_local;
                alias %%TESTDIR%%/ok;
            }
        }
    }
}

EOF

$t->write_file('ok', 'ok');
$t->run();

###############################################################################

like(http_get('/basic'), qr/200 OK/, 'no matching rule');
like(http_get('/basic?deny=1'), qr/403 Forbidden/, 'deny variable');
like(http_get('/basic?allow=1&deny=1'), qr/200 OK/,
	'first matching allow wins');

like(http_get('/when'), qr/200 OK/, 'when rule miss');
like(http_get('/when?block=yes'), qr/403 Forbidden/, 'when rule hit');

like(http_get('/status'), qr/401 Unauthorized/,
	'unconditional deny status fallback');
like(http_get('/status?status=teapot'), qr/418 /,
	'conditional deny status');

like(http_get('/parent/inherit?parent=1'), qr/403 Forbidden/,
	'inherit parent rules');
like(http_get('/parent/on?parent=1'), qr/200 OK/,
	'default inheritance keeps local rules');
like(http_get('/parent/before?parent=1'), qr/403 Forbidden/,
	'parent rules before local rules');
like(http_get('/parent/after?parent=1'), qr/200 OK/,
	'parent rules after local rules');
like(http_get('/parent/off?parent=1'), qr/200 OK/,
	'disable parent rule inheritance');

###############################################################################

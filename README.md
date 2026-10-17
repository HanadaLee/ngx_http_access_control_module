# ngx_http_access_control_module

# Name

A custom Nginx module for advanced access control based on variables.

# Table of Content

- [ngx\_http\_access\_control\_module](#ngx_http_access_control_module)
- [Name](#name)
- [Table of Content](#table-of-content)
- [Status](#status)
- [Synopsis](#synopsis)
- [Installation](#installation)
- [Directives](#directives)
  - [access](#access)
  - [access\_inherit](#access_inherit)
  - [access\_deny\_status](#access_deny_status)
- [Author](#author)
- [License](#license)

# Status

This Nginx module is currently considered experimental. Issues and PRs are welcome if you encounter any problems.

# Synopsis

```nginx
server {
    listen 80;
    server_name example.com;

    # Allow access if $var1 is non-empty and not zero. The allowed request will no longer match the remaining access control rules.
    access allow $var1;

    # Deny access if $var2 is non-empty and not zero
    access deny $var2;

    # Unconditional deny
    access deny;

    location / {
        # Your other configurations
    }

    location /restricted {
        # Override deny status code
        access_deny_status 404;

        # Deny access if $var3 is non-empty and not zero
        access deny $var3;

        # Deny access conditionally with a when block
        when ($remote_addr ~ "192\.168\.") {
            access deny;
        }
    }
}
```

# Installation

To use theses modules, configure your nginx branch with `--add-module=/path/to/ngx_http_access_control_module`.

if [ngx_expr_module](https://git.hanada.info/hanada/ngx_condition_module) is used, the module will use the `ngx_expr_module`'s condition evaluation logic.

# Directives

## access

**Syntax:** *access allow | deny [variable];*

**Default:** *-*

**Context:** *http, server, location, http when, server when, location when*

The access directive defines an access control rule based on a variable. The variable is evaluated at runtime, and if it is non-empty and not zero, the rule is considered matched.

The `allow` parameter allows access if the condition is met. The allowed request will no longer match the remaining access control rules.
The `deny`  parameter denies access if the condition is met.

if variable is not defined, the rule is always considered matched.
```nginx
access deny;
```

## access_inherit

**Syntax:** *access_inherit on | off | before | after;*

**Default:** *access_inherit on;*

**Context:** *http, server, location*

Controls inheritance of `access` rules. With `on`, the previous configuration
level is used when no local rules are defined or, with `ngx_expr_module`, all
local `when` conditions are false. An unconditional rule or a matching `when`
selects the local group, even if its variable parameter is empty or exactly
`0`. A false variable parameter skips that rule; it does not cause `on` to
fall back to the parent.

The `before` parameter evaluates the parent's effective rules before the local
rules. The `after` parameter evaluates them after the local rules if no local
allow or deny matches. Each parent retains its own inheritance mode.

The first matching allow or deny ends this module's evaluation. An allow does
not bypass other access-control modules. Inherited conditions are evaluated
using the current configuration's expression definitions.

The `off` parameter cancels inheritance of the values from the previous configuration level.

## access_deny_status

**Syntax:** *access_deny_status code;*

**Default:** *access_deny_status 403;*

**Context:** *http, server, location, http when, server when, location when*

Sets the HTTP status code to return in response when access is denied by a deny rule.
The current location's setting also applies when the matching deny rule is inherited.

# Author

Hanada im@hanada.info

# License

This Nginx module is licensed under [BSD 2-Clause License](LICENSE).

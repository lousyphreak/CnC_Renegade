#!/bin/sh

set -eu

port="${NGINX_PORT:-80}"
auth_user="${HTTP_BASIC_AUTH_USERNAME:-}"
auth_password="${HTTP_BASIC_AUTH_PASSWORD:-}"
auth_realm="${HTTP_BASIC_AUTH_REALM:-Renegade}"

if [ -n "${auth_user}" ] || [ -n "${auth_password}" ]; then
    if [ -z "${auth_user}" ] || [ -z "${auth_password}" ]; then
        echo "Set both HTTP_BASIC_AUTH_USERNAME and HTTP_BASIC_AUTH_PASSWORD to enable HTTP basic auth." >&2
        exit 1
    fi

    escaped_realm="$(printf '%s' "${auth_realm}" | sed 's/\\/\\\\/g; s/"/\\"/g')"
    htpasswd -bc /etc/nginx/.htpasswd "${auth_user}" "${auth_password}" >/dev/null
    auth_directives="    auth_basic \"${escaped_realm}\";
    auth_basic_user_file /etc/nginx/.htpasswd;"
else
    rm -f /etc/nginx/.htpasswd
    auth_directives="    auth_basic off;"
fi

cat > /etc/nginx/conf.d/default.conf <<EOF
server {
    listen ${port};
    listen [::]:${port};
    server_name _;

    root /usr/share/nginx/html;
    index Renegade.html;

${auth_directives}

    add_header Cache-Control "no-cache, no-store, must-revalidate" always;
    add_header Pragma "no-cache" always;
    add_header Expires "0" always;
    add_header Accept-Ranges "bytes" always;

    location = / {
        try_files /Renegade.html =404;
    }

    location / {
        try_files \$uri \$uri/ =404;
    }

    location ~ \.wasm$ {
        default_type application/wasm;
        try_files \$uri =404;
    }
}
EOF

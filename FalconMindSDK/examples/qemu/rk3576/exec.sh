#!/bin/bash
#
# Execute command in RK3576 QEMU VM
# 在RK3576 QEMU虚拟机中执行命令
#

set -e

VM_HOST="localhost"
VM_PORT="2222"
VM_USER="falcon"
VM_PASS="falcon"

if [[ $# -gt 0 ]]; then
    # 执行指定命令
    sshpass -p "$VM_PASS" ssh -o StrictHostKeyChecking=no "$VM_USER@$VM_HOST" -p "$VM_PORT" "$@"
else
    # 打开交互式shell
    sshpass -p "$VM_PASS" ssh -o StrictHostKeyChecking=no "$VM_USER@$VM_HOST" -p "$VM_PORT"
fi

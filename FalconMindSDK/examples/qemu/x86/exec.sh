#!/bin/bash
#
# Execute command in x86 QEMU VM
#

VM_HOST="localhost"
VM_PORT="2222"
VM_USER="falcon"
VM_PASS="falcon"

if [[ $# -gt 0 ]]; then
    sshpass -p "$VM_PASS" ssh -o StrictHostKeyChecking=no "$VM_USER@$VM_HOST" -p "$VM_PORT" "$@"
else
    sshpass -p "$VM_PASS" ssh -o StrictHostKeyChecking=no "$VM_USER@$VM_HOST" -p "$VM_PORT"
fi

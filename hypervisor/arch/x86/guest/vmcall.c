/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>
#include <hypercall.h>

/*
 * Pass return value to SOS by register rax.
 * This function should always return 0 since we shouldn't
 * deal with hypercall error in hypervisor.
 */
int vmcall_vmexit_handler(struct vcpu *vcpu)
{
	int32_t ret = -EACCES;
	struct vm *vm = vcpu->vm;
	struct run_context *cur_context =
		&vcpu->arch_vcpu.contexts[vcpu->arch_vcpu.cur_context];
	/* hypercall ID from guest*/
	uint64_t hypcall_id = cur_context->guest_cpu_regs.regs.r8;
	/* hypercall param1 from guest*/
	uint64_t param1 = cur_context->guest_cpu_regs.regs.rdi;
	/* hypercall param2 from guest*/
	uint64_t param2 = cur_context->guest_cpu_regs.regs.rsi;
	/* hypercall param3 from guest, reserved*/
	/* uint64_t param3 = cur_context->guest_cpu_regs.regs.rdx; */
	/* hypercall param4 from guest, reserved*/
	/* uint64_t param4 = cur_context->guest_cpu_regs.regs.rcx; */

	if (!is_hypercall_from_ring0()) {
		pr_err("hypercall is only allowed from RING-0!\n");
		goto out;
	}

	if (!is_vm0(vm) && hypcall_id != HC_WORLD_SWITCH &&
		hypcall_id != HC_INITIALIZE_TRUSTY) {
		pr_err("hypercall %d is only allowed from VM0!\n", hypcall_id);
		goto out;
	}

	/* Dispatch the hypercall handler */
	switch (hypcall_id) {
	case HC_GET_API_VERSION:
		/* vm0 will call HC_GET_API_VERSION as first hypercall, fixup
		 * vm0 vcpu here.
		 */
		vm_fixup(vm);
		ret = hcall_get_api_version(vm, param1);
		break;

	case HC_CREATE_VM:
		ret = hcall_create_vm(vm, param1);
		break;

	case HC_DESTROY_VM:
		/* param1: vmid */
		ret = hcall_destroy_vm((uint16_t)param1);
		break;

	case HC_START_VM:
		/* param1: vmid */
		ret = hcall_resume_vm((uint16_t)param1);
		break;

	case HC_PAUSE_VM:
		/* param1: vmid */
		ret = hcall_pause_vm((uint16_t)param1);
		break;

	case HC_CREATE_VCPU:
		/* param1: vmid */
		ret = hcall_create_vcpu(vm, (uint16_t)param1, param2);
		break;

	case HC_ASSERT_IRQLINE:
		/* param1: vmid */
		ret = hcall_assert_irqline(vm, (uint16_t)param1, param2);
		break;

	case HC_DEASSERT_IRQLINE:
		/* param1: vmid */
		ret = hcall_deassert_irqline(vm, (uint16_t)param1, param2);
		break;

	case HC_PULSE_IRQLINE:
		/* param1: vmid */
		ret = hcall_pulse_irqline(vm, (uint16_t)param1, param2);
		break;

	case HC_INJECT_MSI:
		/* param1: vmid */
		ret = hcall_inject_msi(vm, (uint16_t)param1, param2);
		break;

	case HC_SET_IOREQ_BUFFER:
		/* param1: vmid */
		ret = hcall_set_ioreq_buffer(vm, (uint16_t)param1, param2);
		break;

	case HC_NOTIFY_REQUEST_FINISH:
		/* param1: vmid
		 * param2: vcpu_id */
		ret = hcall_notify_req_finish((uint16_t)param1,
			(uint16_t)param2);
		break;

	case HC_VM_SET_MEMMAP:
		/* param1: vmid */
		ret = hcall_set_vm_memmap(vm, (uint16_t)param1, param2);
		break;

	case HC_VM_SET_MEMMAPS:
		ret = hcall_set_vm_memmaps(vm, param1);
		break;

	case HC_VM_PCI_MSIX_REMAP:
		/* param1: vmid */
		ret = hcall_remap_pci_msix(vm, (uint16_t)param1, param2);
		break;

	case HC_VM_GPA2HPA:
		/* param1: vmid */
		ret = hcall_gpa_to_hpa(vm, (uint16_t)param1, param2);
		break;

	case HC_ASSIGN_PTDEV:
		/* param1: vmid */
		ret = hcall_assign_ptdev(vm, (uint16_t)param1, param2);
		break;

	case HC_DEASSIGN_PTDEV:
		/* param1: vmid */
		ret = hcall_deassign_ptdev(vm, (uint16_t)param1, param2);
		break;

	case HC_SET_PTDEV_INTR_INFO:
		/* param1: vmid */
		ret = hcall_set_ptdev_intr_info(vm, (uint16_t)param1, param2);
		break;

	case HC_RESET_PTDEV_INTR_INFO:
		/* param1: vmid */
		ret = hcall_reset_ptdev_intr_info(vm, (uint16_t)param1, param2);
		break;

	case HC_SETUP_SBUF:
		ret = hcall_setup_sbuf(vm, param1);
		break;

	case HC_WORLD_SWITCH:
		ret = hcall_world_switch(vcpu);
		break;

	case HC_INITIALIZE_TRUSTY:
		ret = hcall_initialize_trusty(vcpu, param1);
		break;

	case HC_PM_GET_CPU_STATE:
		ret = hcall_get_cpu_pm_state(vm, param1, param2);
		break;

	default:
		pr_err("op %d: Invalid hypercall\n", hypcall_id);
		ret = -EPERM;
		break;
	}

out:
	cur_context->guest_cpu_regs.regs.rax = (uint64_t)ret;

	TRACE_2L(TRACE_VMEXIT_VMCALL, vm->attr.id, hypcall_id);

	return 0;
}

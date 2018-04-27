; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=x86_64-unknown-unknown -mattr=+tbm < %s | FileCheck %s

define i32 @test_x86_tbm_bextri_u32(i32 %a) nounwind readnone {
; CHECK-LABEL: test_x86_tbm_bextri_u32:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrl $2814, %edi, %eax # imm = 0xAFE
; CHECK-NEXT:    retq
entry:
  %0 = tail call i32 @llvm.x86.tbm.bextri.u32(i32 %a, i32 2814)
  ret i32 %0
}

declare i32 @llvm.x86.tbm.bextri.u32(i32, i32) nounwind readnone

define i32 @test_x86_tbm_bextri_u32_m(i32* nocapture %a) nounwind readonly {
; CHECK-LABEL: test_x86_tbm_bextri_u32_m:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrl $2814, (%rdi), %eax # imm = 0xAFE
; CHECK-NEXT:    retq
entry:
  %tmp1 = load i32, i32* %a, align 4
  %0 = tail call i32 @llvm.x86.tbm.bextri.u32(i32 %tmp1, i32 2814)
  ret i32 %0
}

define i32 @test_x86_tbm_bextri_u32_z(i32 %a, i32 %b) nounwind readonly {
; CHECK-LABEL: test_x86_tbm_bextri_u32_z:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrl $2814, %edi, %eax # imm = 0xAFE
; CHECK-NEXT:    cmovel %esi, %eax
; CHECK-NEXT:    retq
entry:
  %0 = tail call i32 @llvm.x86.tbm.bextri.u32(i32 %a, i32 2814)
  %1 = icmp eq i32 %0, 0
  %2 = select i1 %1, i32 %b, i32 %0
  ret i32 %2
}

define i64 @test_x86_tbm_bextri_u64(i64 %a) nounwind readnone {
; CHECK-LABEL: test_x86_tbm_bextri_u64:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrq $2814, %rdi, %rax # imm = 0xAFE
; CHECK-NEXT:    retq
entry:
  %0 = tail call i64 @llvm.x86.tbm.bextri.u64(i64 %a, i64 2814)
  ret i64 %0
}

declare i64 @llvm.x86.tbm.bextri.u64(i64, i64) nounwind readnone

define i64 @test_x86_tbm_bextri_u64_m(i64* nocapture %a) nounwind readonly {
; CHECK-LABEL: test_x86_tbm_bextri_u64_m:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrq $2814, (%rdi), %rax # imm = 0xAFE
; CHECK-NEXT:    retq
entry:
  %tmp1 = load i64, i64* %a, align 8
  %0 = tail call i64 @llvm.x86.tbm.bextri.u64(i64 %tmp1, i64 2814)
  ret i64 %0
}

define i64 @test_x86_tbm_bextri_u64_z(i64 %a, i64 %b) nounwind readnone {
; CHECK-LABEL: test_x86_tbm_bextri_u64_z:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    bextrq $2814, %rdi, %rax # imm = 0xAFE
; CHECK-NEXT:    cmoveq %rsi, %rax
; CHECK-NEXT:    retq
entry:
  %0 = tail call i64 @llvm.x86.tbm.bextri.u64(i64 %a, i64 2814)
  %1 = icmp eq i64 %0, 0
  %2 = select i1 %1, i64 %b, i64 %0
  ret i64 %2
}

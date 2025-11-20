/* Sanjay Rai - Routine to access PCIe via  dev.mem mmap  */
#ifndef SRAI_ACCEL_UTILS_H_
#define SRAI_ACCEL_UTILS_H_

#define PAGE_SIZE (4*1024UL*1024UL)

/* Address Ranges as defined in VIvado IPI Address map */
/* NOTE: Be aware on any PCIe-AXI Address translation setup on the xDMA/PCIEBridge*/ 
/*       These address translations affect the address shown . THese address are  */
/*       exactly waht is populated on the IPI Address tab                         */
/* AXI MM Register interfaces */
#define AXI_MM_DDR4_C0         0x0000020000ULL
#define AXI_MM_DDR4_results_C0 0x0000030000ULL
#define AXI_MM_DDR4_C1         0x0000000000ULL
#define AXI_MM_DDR4_results_C1 0x0000010000ULL
#define AXI_MM_DDR4_C2         0x0000040000ULL
#define AXI_MM_DDR4_results_C2 0x0000050000ULL
#define AXI_MM_DDR4_C3         0x0000060000ULL
#define AXI_MM_DDR4_results_C3 0x0000070000ULL


/* AXI LITE Register interfaces */
#define AXI_LITE_GPIO_BASE             0x00010000UL
#define AXI_LITE_PROG_CLOCK_BASE       0x00020000UL
#define AXI_LITE_ICAP_BASE             0x00030000UL
#define AXI_LITE_PR_HLS_NORTH_BASE     0x00100000UL
#define AXI_LITE_AXI_SYSMON_BASE       0x00050000UL
#define AXI_LITE_AXI_STATUS            0x00080000UL

/* Address Offset of various Peripheral Registers */ 
#define ICAP_CNTRL_REG              AXI_LITE_ICAP_BASE + 0x10CUL
#define ICAP_STATUS_REG             AXI_LITE_ICAP_BASE + 0x110UL
#define ICAP_WR_FIFO_REG            AXI_LITE_ICAP_BASE + 0x100UL
#define ICAP_WR_FIFO_VACANCY_REG    AXI_LITE_ICAP_BASE + 0x114UL
#define PR_HLS_NORTH_CONTROL_REG          AXI_LITE_PR_HLS_NORTH_BASE + 0x0UL
#define PR_HLS_NORTH_STATUS_REG           AXI_LITE_PR_HLS_NORTH_BASE + 0x4UL
#define PR_HLS_NORTH_INPUT_BYTES_L        AXI_LITE_PR_HLS_NORTH_BASE + 0x8UL
#define PR_HLS_NORTH_INPUT_BYTES_H        AXI_LITE_PR_HLS_NORTH_BASE + 0xCUL
#define PR_HLS_NORTH_OUTPUT_BYTES_L       AXI_LITE_PR_HLS_NORTH_BASE + 0x10UL
#define PR_HLS_NORTH_OUTPUT_BYTES_H       AXI_LITE_PR_HLS_NORTH_BASE + 0x14UL
#define PROG_CLOCK_STATUS_REGISTER        AXI_LITE_PROG_CLOCK_BASE + 0x004UL
#define PROG_CLOCK_DIVIDE_REGISTER        AXI_LITE_PROG_CLOCK_BASE + 0x208UL
#define PROG_CLOCK_CONTROL_REGISTER       AXI_LITE_PROG_CLOCK_BASE + 0x25CUL
#define SYSMON_Temptature_register        AXI_LITE_AXI_SYSMON_BASE + 0x400UL 
#define SYSMON_MAX_Temptature_register    AXI_LITE_AXI_SYSMON_BASE + 0x1c80L 
#define SYSMON_MIN_Temptature_register    AXI_LITE_AXI_SYSMON_BASE + 0x1c90L 

#define ISOLATE_NORTH_PR     0x00000000UL
#define DEISOLATE_NORTH_PR   0xFFFFFFFFUL

typedef struct {
    double current_temp;
    double maximum_temp;
    double minimum_temp;
} SysMon_temp_struct;

typedef struct {
    uint32_t KERNEL_DATASET;
    uint32_t KERNEL_CLOCK_COUNT;
    double KERNEL_EXECUTION_TIME;
} kernel_execution_metric_struct;

int fpga_clean (fpga_xDMA_linux *my_fpga_xDMA_ptr);
void fpga_PCIE_BANDWIDTH_test64 (fpga_xDMA_linux *my_fpga_xDMA_ptr, uint64_t AXI_ADDRESS,  char *A_IN, uint32_t xfer_size);
void fpga_PROGRAM_NORTH_PR (fpga_xDMA_linux *fpga_AXI_Lite_ptr, string PR_binFile_name);
void fpga_xfer_data_to_card64(fpga_xDMA_linux *my_fpga_xDMA_ptr, uint64_t AXI_ADDRESS, char *data_buf_ptr, uint32_t XFER_SIZE);
void fpga_xfer_data_from_card64(fpga_xDMA_linux *my_fpga_xDMA_ptr, uint64_t AXI_ADDRESS_RESULTS, char *data_buf_ptr, uint32_t XFER_SIZE);
void fpga_run_NORTH_PR64(fpga_xDMA_linux *fpga_AXI_Lite_ptr, uint64_t AXI_ADDRESS_in0, uint64_t AXI_ADDRESS_RESULTS, uint64_t NUM_OF_DATA_SETS);
uint64_t  fpga_check_completion_bytes_NORTH_PR (fpga_xDMA_linux *fpga_AXI_Lite_ptr);
int  fpga_check_compute_done_NORTH_PR (fpga_xDMA_linux *fpga_AXI_Lite_ptr);
void fpga_test_AXIL_LITE_8KSCRATCHPAD_BRAM (fpga_xDMA_linux *fpga_AXI_Lite_ptr);
void ICAP_prog_PR_binfile (fpga_xDMA_linux *fpga_AXI_Lite_ptr, string PR_binFile_name);
void xDMA_througput_h2c64 (fpga_xDMA_linux *my_fpga_xDMA_ptr, uint64_t AXI_ADDRESS, char *data_buffer, uint32_t xfer_size);
void xDMA_througput_c2h64 (fpga_xDMA_linux *my_fpga_xDMA_ptr, uint64_t AXI_ADDRESS, char *data_buffer, uint32_t xfer_size);
void fpga_PROGRAM_PR_CLOCK (fpga_xDMA_linux *fpga_AXI_Lite_ptr, float PR_Frequency);
void fpga_read_temprature (fpga_xDMA_linux *fpga_AXI_Lite_ptr, SysMon_temp_struct *sys_temp, int average_num);
void fpga_get_Kernel_execution_time (fpga_xDMA_linux *fpga_AXI_Lite_ptr, double kernel_operating_frequency, kernel_execution_metric_struct *kernel_execution_metric);
void fpga_run_kernel(fpga_xDMA_linux *fpga_AXI_Lite_ptr);
void fpga_set_kernel_arguments_32(fpga_xDMA_linux *fpga_AXI_Lite_ptr, uint32_t Addr_offset, uint32_t data);
void fpga_set_kernel_arguments_64(fpga_xDMA_linux *fpga_AXI_Lite_ptr, uint32_t Addr_offset, uint64_t data);
void SRAI_dbg_wait(string dbg_string);
uint32_t  fpga_read_status (fpga_xDMA_linux *fpga_AXI_Lite_ptr);
#endif // SRAI_ACCEL_UTILS_H_

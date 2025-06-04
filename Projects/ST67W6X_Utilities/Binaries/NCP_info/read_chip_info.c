#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct
{
  uint32_t en_addr;       /*!< EFUSE Trim Enable Address */
  uint32_t en_offset;     /*!< EFUSE Trim Enable Offset */
  uint32_t value_addr;    /*!< EFUSE Trim Value Address */
  uint32_t value_offset;  /*!< EFUSE Trim Value Offset */
  uint32_t value_len;     /*!< EFUSE Trim Value Length */
  uint32_t parity_addr;   /*!< EFUSE Trim Parity Address */
  uint32_t parity_offset; /*!< EFUSE Trim Parity Offset */
  uint32_t type;          /*!< Trim type */
  const char *desc;       /*!< Trim Description */
} trim_table_t;

typedef struct
{
  char *name;
  uint32_t offset;
  uint32_t start_bit;
  uint32_t end_bit;
  uint32_t size;
  uint32_t byte_order;
} efuse_field_t;

#define EFUSE_SIZE 512

efuse_field_t efuse_fields[] = {
    {"JTAG", 0x00, 27, 26, 0, 0},
    {"Public Key", 0x1C, 0, 0, 32, 0},
    {"Anti-rollback enable", 0x7C, 12, 12, 0, 0},
    {"Anti-rollback Bootloader", 0x170, 0, 0, 16, 0},
    {"Anti-rollback Application", 0x180, 0, 0, 32, 0},
    {"Default MAC Address", 0x14, 0, 0, 6, 1},
    {"Customer MAC Address1", 0x64, 0, 0, 6, 1},
    {"Customer MAC Address2", 0x70, 0, 0, 6, 1},
    {"Part Number", 0x100, 0, 0, 24, 0},
    {"Manufacturing Year/Week", 0x11A, 0, 0, 2, 0},
    {"BOM Id", 0x118, 0, 0, 2, 0},
};

trim_table_t trim_table[] =
    {
        {0xCC, 26, 0xC0, 0, 15, 0xC0, 15, 0, "wifi_hp_poffset0"},
        {0xCC, 27, 0xC0, 16, 15, 0xC0, 31, 0, "wifi_hp_poffset1"},
        {0xCC, 28, 0xC4, 0, 15, 0xC4, 15, 0, "wifi_hp_poffset2"},
        {0xCC, 29, 0xC4, 16, 15, 0xC4, 31, 1, "wifi_lp_poffset0"},
        {0xCC, 30, 0xC8, 0, 15, 0xC8, 15, 1, "wifi_lp_poffset1"},
        {0xCC, 31, 0xC8, 16, 15, 0xC8, 31, 1, "wifi_lp_poffset2"},
        {0xD0, 26, 0xCC, 0, 25, 0xCC, 25, 2, "ble_poffset0"},
        {0xD0, 27, 0xD0, 0, 25, 0xD0, 25, 2, "ble_poffset1"},
        {0xD0, 28, 0xD4, 0, 25, 0xD4, 25, 2, "ble_poffset2"},
        {0xEC, 7, 0XEC, 0, 6, 0XEC, 6, 3, "xtal0"},
        {0xF0, 31, 0XF4, 26, 6, 0XF0, 30, 3, "xtal1"},
        {0xEC, 23, 0XF4, 20, 6, 0XF0, 28, 3, "xtal2"},
};

unsigned int countSetBits(unsigned int n)
{
  return __builtin_popcount(n);
}

uint32_t extract_bits(uint32_t value, int start_bit, int end_bit)
{
  uint32_t mask = (1 << (end_bit - start_bit + 1)) - 1;
  return (value >> start_bit) & mask;
}

// Function to reverse a byte array
void reverse_bytes(uint8_t *start, int size)
{
  int i, j;
  uint8_t temp;
  for (i = 0, j = size - 1; i < j; i++, j--)
  {
    temp = start[i];
    start[i] = start[j];
    start[j] = temp;
  }
}

// Function to convert hex string to ASCII
void hex_to_ascii(char *hex_str, char *ascii_str)
{
  while (*hex_str)
  {
    char byte[3] = {hex_str[0], hex_str[1], '\0'};
    *ascii_str++ = (char)strtol(byte, NULL, 16);
    hex_str += 2;
  }
  *ascii_str = '\0';
}

// Function to process the efuse data
void process_efuse_data(uint8_t *efuse_data, int addr, int size, int order, char *field, char *parsed_data)
{
  char hex_str[256];

  if (order == 1)
  {
    reverse_bytes(&efuse_data[addr], size);
  }

  for (int i = 0; i < size; i++)
  {
    sprintf(&hex_str[i * 2], "%02X", efuse_data[addr + i]);
  }

  if (strstr(field, "MAC Address"))
  {
    for (int i = 0; i < size; i++)
    {
      sprintf(&parsed_data[i * 3], "%02X:", efuse_data[addr + i]);
    }
    parsed_data[size * 3 - 1] = '\0'; // Remove the last colon
  }
  else if (strstr(field, "Anti-rollback"))
  {
    // Get the most significant bit from the hex string
    int value = 0;
    for (int i = 0; i < size / 8; i++)
    {
      // builtin_clz returns the number of leading zeros in the binary representation of the input
      value += __builtin_popcount(*((uint32_t *)(efuse_data + addr) + i));
    }
    sprintf(parsed_data, "%d", value);
  }
  else if (strstr(field, "Part Number"))
  {
    char ascii_str[25] = {0};
    memcpy(ascii_str, &efuse_data[addr], size);
    if (strstr(ascii_str, "\x03"))
    {
      *strstr(ascii_str, "\x03") = '\0';
      strcpy(parsed_data, ascii_str);
    }
    else
    {
      strcpy(parsed_data, "Invalid Part Number");
    }
  }
  else if (strstr(field, "Manufacturing"))
  {
    uint32_t value = strtoul(hex_str, NULL, 16);
    int year = (value >> 8) & 0xFF;
    int week = value & 0xFF;
    sprintf(parsed_data, "Year 20%02d | Week %02d", year, week);
  }
  else if (strstr(field, "BOM Id"))
  {
    uint32_t value = strtoul(hex_str, NULL, 16);
    sprintf(parsed_data, "%04X", value);
  }
  else if (strstr(field, "Public Key"))
  {
    uint32_t empty = 1;
    for (int i = 0; i < size; i++)
    {
      if (efuse_data[addr + i] != 0)
      {
        empty = 0;
        break;
      }
    }
    sprintf(parsed_data, "%s", empty == 1 ? "Empty" : "Filled");
  }
  else
  {
    hex_to_ascii(hex_str, parsed_data);
  }
}

// Function to process trimming values
void process_trimming_values(uint8_t *efuse_data, trim_table_t *trim_table, int trim_table_size)
{
  int8_t trim_wifi[14];
  int8_t trim_ble[5];
  int8_t trim_xtal;

  for (int i = 0; i < trim_table_size; i++)
  {
    uint32_t en_value = *(uint32_t *)&efuse_data[trim_table[i].en_addr];
    if ((en_value >> trim_table[i].en_offset) & 0x1)
    {
      uint32_t data[2] = {0};
      data[0] = *(uint32_t *)&efuse_data[trim_table[i].parity_addr];
      data[1] = *(uint32_t *)&efuse_data[trim_table[i].value_addr];
      uint32_t cnt = 0;
      int32_t k;
      int32_t step = 0;
      int8_t pwr_offset[14];
      int8_t pwr_offset_tmp[3];

      /* Parity */
      uint32_t trim_parity = data[0] >> trim_table[i].parity_offset & 0x1;

      /* Trim offset */
      uint32_t trim_value = data[1] >> trim_table[i].value_offset & ((1 << trim_table[i].value_len) - 1);

      for (k = 0; k < trim_table[i].value_len; k++) /* Count number of bits set */
      {
        if (trim_value & (1 << k))
        {
          cnt++;
        }
      }

      if ((cnt & 0x1) != trim_parity) /* Check parity */
      {
        continue;
      }

      if (trim_table[i].type == 2) /* BLE */
      {
        for (k = 0; k < 5; k++)
        {
          /* Calculate the 5 channels offset from the 25 bits value */
          trim_ble[k] = (trim_value >> (k * 5)) & 0x1f;
          if (trim_ble[k] >= 16)
          {
            trim_ble[k] -= 32;
          }
        }
        printf("%-30s : [%d,%d,%d,%d,%d]\n", trim_table[i].desc, trim_ble[0], trim_ble[1], trim_ble[2], trim_ble[3], trim_ble[4]);
      }
      else if ((trim_table[i].type == 0) || (trim_table[i].type == 1)) /* Wi-Fi */
      {
        /* Calculate the 14 channels offset from the 15 bits value */
        for (k = 0; k < 3; k++)
        {
          pwr_offset_tmp[k] = (trim_value >> (k * 5)) & 0x1f;

          if (pwr_offset_tmp[k] >= 16)
          {
            pwr_offset_tmp[k] -= 32;
          }
        }

        pwr_offset[0] = pwr_offset_tmp[0];
        pwr_offset[6] = pwr_offset_tmp[1];
        pwr_offset[12] = pwr_offset_tmp[2];

        step = (pwr_offset_tmp[1] - pwr_offset_tmp[0]) * 100 / 6;
        for (k = 1; k < 6; k++)
        {
          pwr_offset[k] = ((step * k) + 50) / 100 + pwr_offset_tmp[0];
        }

        step = (pwr_offset_tmp[2] - pwr_offset_tmp[1]) * 100 / 6;
        for (k = 7; k < 12; k++)
        {
          pwr_offset[k] = ((step * (k - 6)) + 50) / 100 + pwr_offset_tmp[1];
        }

        pwr_offset[13] = (step * 7 + 50) / 100 + pwr_offset_tmp[1];

        if (trim_table[i].type == 0) /* Wi-Fi high-performance */
        {
          memcpy(trim_wifi, pwr_offset, sizeof(trim_wifi));
        }
        else /* Wi-Fi low-power */
        {
          memcpy(trim_wifi, pwr_offset, sizeof(trim_wifi));
        }
        printf("%-30s : [%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]\n", trim_table[i].desc, trim_wifi[0], trim_wifi[1], trim_wifi[2], trim_wifi[3], trim_wifi[4], trim_wifi[5], trim_wifi[6], trim_wifi[7], trim_wifi[8], trim_wifi[9], trim_wifi[10], trim_wifi[11], trim_wifi[12], trim_wifi[13]);
      }
      else if (trim_table[i].type == 3) /* XTAL */
      {
        /* LogInfo("%s: %d", trim_table[i].desc, trim_value); */
        trim_xtal = trim_value;
        printf("%-30s : %d\n", trim_table[i].desc, trim_xtal);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <path_to_binary_file>\n", argv[0]);
    return 1;
  }

  const char *file_path = argv[1];
  FILE *file = fopen(file_path, "rb");
  if (!file)
  {
    perror("Failed to open file");
    return 1;
  }

  uint8_t efuse_data[EFUSE_SIZE];
  size_t read_size = fread(efuse_data, 1, EFUSE_SIZE, file);
  fclose(file);

  if (read_size != EFUSE_SIZE)
  {
    fprintf(stderr, "Failed to read e-Fuse data\n");
    return 1;
  }

  // display efuse data
  //  for (int i = 0; i < EFUSE_SIZE; i++) {
  //      printf("%02X ", efuse_data[i]);
  //  }

  for (int i = 0; i < sizeof(efuse_fields) / sizeof(efuse_field_t); i++)
  {
    char parsed_data[256];
    memset(parsed_data, 0, sizeof(parsed_data));
    if (efuse_fields[i].start_bit != 0)
    {
      uint32_t value = extract_bits(*(uint32_t *)(efuse_data + efuse_fields[i].offset), efuse_fields[i].end_bit, efuse_fields[i].start_bit);

      if (strstr(efuse_fields[i].name, "JTAG"))
      {
        sprintf(parsed_data, "%s", value == 3 ? "Disabled" : "Enabled");
      }
      else if (strstr(efuse_fields[i].name, "Anti-rollback"))
      {
        sprintf(parsed_data, "%s", value == 1 ? "Enabled" : "Disabled");
      }
      else
      {
        sprintf(parsed_data, "%d", value);
      }
    }
    else
    {
      process_efuse_data(efuse_data, efuse_fields[i].offset, efuse_fields[i].size, efuse_fields[i].byte_order, efuse_fields[i].name, parsed_data);
    }
    printf("%-30s : %s\n", efuse_fields[i].name, parsed_data);
  }

  process_trimming_values(efuse_data, trim_table, sizeof(trim_table) / sizeof(trim_table_t));

  return 0;
}

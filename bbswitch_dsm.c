#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/seq_file.h>

static acpi_handle bbswitch_dsm_handle;

static const char acpi_optimus_dsm_muid[16] = {
    0xF8, 0xD8, 0x86, 0xA4, 0xDA, 0x0B, 0x1B, 0x47,
    0xA7, 0x2B, 0x60, 0x42, 0xA6, 0xB5, 0xBE, 0xE0,
};

static char *buffer_to_string(const char *buffer, size_t n, char *target) {
    int i;
    for (i=0; i<n; i++) {
        snprintf(target + i * 5, 5 * (n - i), "0x%02X,", buffer ? buffer[i] & 0xFF : 0);
    }
    return target;
}

// Returns 0 if the call succeeded and non-zero otherwise. If the call
// succeeded, the result is stored in "result" providing that the result is an
// integer or a buffer containing 4 values
static int acpi_call_dsm(acpi_handle handle, const char muid[16], int revid,
    int func, char args[4], uint32_t *result) {
    struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
    struct acpi_object_list input;
    union acpi_object params[4];
    union acpi_object *obj;
    int err;

    input.count = 4;
    input.pointer = params;
    params[0].type = ACPI_TYPE_BUFFER;
    params[0].buffer.length = 16;
    params[0].buffer.pointer = (char *)muid;
    params[1].type = ACPI_TYPE_INTEGER;
    params[1].integer.value = revid;
    params[2].type = ACPI_TYPE_INTEGER;
    params[2].integer.value = func;
    /* Although the ACPI spec defines Arg3 as a Package, in practise
     * implementations expect a Buffer (CreateWordField and Index functions are
     * applied to it). */
    params[3].type = ACPI_TYPE_BUFFER;
    params[3].buffer.length = 4;
    if (args) {
        params[3].buffer.pointer = args;
    } else {
        // Some implementations (Asus U36SD) seem to check the args before the
        // function ID and crash if it is not a buffer.
        params[3].buffer.pointer = (char[4]){0, 0, 0, 0};
    }

    err = acpi_evaluate_object(handle, "_DSM", &input, &output);
    if (err) {
        struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER, NULL };
        char muid_str[5 * 16];
        char args_str[5 * 4];

        acpi_get_name(handle, ACPI_FULL_PATHNAME, &buf);

        pr_warn("failed to evaluate %s._DSM {%s} 0x%X 0x%X {%s}: %s\n",
            (char *)buf.pointer,
            buffer_to_string(muid, 16, muid_str), revid, func,
            buffer_to_string(args,  4, args_str), acpi_format_exception(err));
        return err;
    }

    obj = (union acpi_object *)output.pointer;

    if (obj->type == ACPI_TYPE_INTEGER && result) {
        *result = obj->integer.value;
    } else if (obj->type == ACPI_TYPE_BUFFER) {
        if (obj->buffer.length == 4 && result) {
            *result = 0;
            *result |= obj->buffer.pointer[0];
            *result |= (obj->buffer.pointer[1] << 8);
            *result |= (obj->buffer.pointer[2] << 16);
            *result |= (obj->buffer.pointer[3] << 24);
        }
    } else {
        pr_warn("_DSM call yields an unsupported result type: %#x\n",
            obj->type);
    }

    kfree(output.pointer);
    return 0;
}

int bbswitch_optimus_dsm(void) {
    char args[] = {1, 0, 0, 3};
    u32 result = 0;

    if (acpi_call_dsm(bbswitch_dsm_handle, acpi_optimus_dsm_muid, 0x100, 0x1A, args,
        &result)) {
        // failure
        return 1;
    }
    pr_debug("Result of Optimus _DSM call: %08X\n", result);
    return 0;
}

void bbswitch_init_dsm(struct device* dev) {
    bbswitch_dsm_handle = ACPI_HANDLE(dev);
}


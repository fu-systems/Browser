/// Script Approval Dialog — shown in "Ask for each script" mode.
///
/// Presents each detected script with a preview and allow/block toggles.
/// Returns the list of [ScriptInfo] with updated [approved] flags.

import 'package:flutter/material.dart';

import '../plugin/built_in/script_manager.dart';

/// Show the approval dialog and return when the user proceeds.
/// Scripts have their [ScriptInfo.approved] flag set by the dialog.
Future<void> showScriptApprovalDialog(
  BuildContext context,
  List<ScriptInfo> scripts,
) {
  return showDialog(
    context: context,
    barrierDismissible: false,
    builder: (_) => _ScriptApprovalDialog(scripts: scripts),
  );
}

class _ScriptApprovalDialog extends StatefulWidget {
  final List<ScriptInfo> scripts;
  const _ScriptApprovalDialog({required this.scripts});

  @override
  State<_ScriptApprovalDialog> createState() => _ScriptApprovalDialogState();
}

class _ScriptApprovalDialogState extends State<_ScriptApprovalDialog> {
  @override
  Widget build(BuildContext context) {
    final scripts = widget.scripts;
    final approvedCount = scripts.where((s) => s.approved).length;

    return Dialog(
      insetPadding: const EdgeInsets.all(32),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 650, maxHeight: 550),
        child: Column(
          children: [
            // ── Header ──
            Container(
              padding: const EdgeInsets.fromLTRB(20, 16, 20, 12),
              decoration: BoxDecoration(
                color: const Color(0xFFFFF3E0),
                borderRadius:
                    const BorderRadius.vertical(top: Radius.circular(12)),
                border: Border(
                  bottom: BorderSide(color: Colors.orange.shade200),
                ),
              ),
              child: Row(
                children: [
                  Icon(Icons.shield_outlined,
                      size: 22, color: Colors.orange.shade700),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text(
                          'JavaScript Approval Required',
                          style: TextStyle(
                            fontSize: 15,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                        Text(
                          '${scripts.length} script${scripts.length == 1 ? '' : 's'} '
                          'detected on this page',
                          style: TextStyle(
                              fontSize: 12, color: Colors.grey.shade600),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),

            // ── Bulk actions ──
            Container(
              padding:
                  const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
              color: const Color(0xFFFAFAFA),
              child: Row(
                children: [
                  Text(
                    '$approvedCount / ${scripts.length} allowed',
                    style: TextStyle(
                        fontSize: 12, color: Colors.grey.shade600),
                  ),
                  const Spacer(),
                  TextButton.icon(
                    onPressed: () {
                      setState(() {
                        for (final s in scripts) {
                          s.approved = true;
                        }
                      });
                    },
                    icon: const Icon(Icons.check_circle_outline, size: 16),
                    label: const Text('Allow All', style: TextStyle(fontSize: 12)),
                  ),
                  const SizedBox(width: 4),
                  TextButton.icon(
                    onPressed: () {
                      setState(() {
                        for (final s in scripts) {
                          s.approved = false;
                        }
                      });
                    },
                    icon: const Icon(Icons.block, size: 16),
                    label: const Text('Block All', style: TextStyle(fontSize: 12)),
                  ),
                ],
              ),
            ),

            // ── Script list ──
            Expanded(
              child: ListView.separated(
                padding: const EdgeInsets.symmetric(vertical: 4),
                itemCount: scripts.length,
                separatorBuilder: (_, __) =>
                    Divider(height: 1, color: Colors.grey.shade200),
                itemBuilder: (context, index) {
                  final script = scripts[index];
                  return _buildScriptRow(script, index);
                },
              ),
            ),

            // ── Footer ──
            Container(
              padding:
                  const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              decoration: BoxDecoration(
                color: const Color(0xFFF5F5F5),
                border: Border(
                  top: BorderSide(color: Colors.grey.shade300),
                ),
              ),
              child: Row(
                children: [
                  Icon(Icons.info_outline,
                      size: 14, color: Colors.grey.shade500),
                  const SizedBox(width: 6),
                  Expanded(
                    child: Text(
                      'Blocked scripts will not execute. External scripts '
                      'without fetched content run as no-ops.',
                      style: TextStyle(
                          fontSize: 11, color: Colors.grey.shade500),
                    ),
                  ),
                  const SizedBox(width: 12),
                  FilledButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Proceed'),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildScriptRow(ScriptInfo script, int index) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Script number.
          SizedBox(
            width: 28,
            child: Text(
              '#${index + 1}',
              style: TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w600,
                color: Colors.grey.shade500,
              ),
            ),
          ),

          // Script details.
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Type badge + source.
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 6, vertical: 2),
                      decoration: BoxDecoration(
                        color: script.isInline
                            ? const Color(0xFFE3F2FD)
                            : const Color(0xFFF3E5F5),
                        borderRadius: BorderRadius.circular(3),
                      ),
                      child: Text(
                        script.isInline ? 'inline' : 'external',
                        style: TextStyle(
                          fontSize: 10,
                          fontWeight: FontWeight.w600,
                          color: script.isInline
                              ? const Color(0xFF1565C0)
                              : const Color(0xFF7B1FA2),
                        ),
                      ),
                    ),
                    const SizedBox(width: 6),
                    if (script.src != null)
                      Expanded(
                        child: Text(
                          script.src!,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            fontSize: 11,
                            color: Colors.grey.shade700,
                            fontFamily: 'monospace',
                          ),
                        ),
                      ),
                    if (script.isInline)
                      Text(
                        '${script.sizeBytes} bytes',
                        style: TextStyle(
                            fontSize: 11, color: Colors.grey.shade500),
                      ),
                  ],
                ),
                const SizedBox(height: 4),
                // Code preview.
                Container(
                  width: double.infinity,
                  padding: const EdgeInsets.all(6),
                  decoration: BoxDecoration(
                    color: const Color(0xFFF5F5F5),
                    borderRadius: BorderRadius.circular(4),
                    border: Border.all(color: Colors.grey.shade300, width: 0.5),
                  ),
                  child: Text(
                    script.preview,
                    maxLines: 3,
                    overflow: TextOverflow.ellipsis,
                    style: const TextStyle(
                      fontSize: 11,
                      fontFamily: 'monospace',
                      color: Color(0xFF37474F),
                      height: 1.3,
                    ),
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(width: 12),

          // Approve/deny toggle.
          Column(
            children: [
              Switch(
                value: script.approved,
                onChanged: (v) {
                  setState(() => script.approved = v);
                },
                activeTrackColor: Colors.green.shade200,
              ),
              Text(
                script.approved ? 'Allow' : 'Block',
                style: TextStyle(
                  fontSize: 10,
                  fontWeight: FontWeight.w600,
                  color: script.approved ? Colors.green : Colors.red.shade400,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

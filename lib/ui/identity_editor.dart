/// Identity Editor — dialog for viewing and editing browser fingerprint
/// profiles. Shows all fingerprint fields organized into collapsible
/// categories, with a dropdown to switch between presets and custom
/// profiles.

import 'package:flutter/material.dart';

import '../plugin/built_in/identity_manager.dart';
import '../plugin/built_in/identity_profile.dart';

/// Opens a full-screen dialog for editing the identity profile.
Future<void> showIdentityEditor(
  BuildContext context,
  IdentityManagerPlugin manager, {
  VoidCallback? onChanged,
}) {
  return showDialog(
    context: context,
    builder: (_) => _IdentityEditorDialog(
      manager: manager,
      onChanged: onChanged,
    ),
  );
}

class _IdentityEditorDialog extends StatefulWidget {
  final IdentityManagerPlugin manager;
  final VoidCallback? onChanged;

  const _IdentityEditorDialog({required this.manager, this.onChanged});

  @override
  State<_IdentityEditorDialog> createState() => _IdentityEditorDialogState();
}

class _IdentityEditorDialogState extends State<_IdentityEditorDialog> {
  late String _selectedProfileName;
  final Map<String, TextEditingController> _controllers = {};
  final Set<String> _expandedCategories = {};

  IdentityManagerPlugin get _mgr => widget.manager;

  @override
  void initState() {
    super.initState();
    _selectedProfileName = _mgr.activeProfile.name;
    _buildControllers();
  }

  @override
  void dispose() {
    for (final c in _controllers.values) {
      c.dispose();
    }
    super.dispose();
  }

  void _buildControllers() {
    for (final c in _controllers.values) {
      c.dispose();
    }
    _controllers.clear();

    for (final cat in profileFieldDefs) {
      for (final field in cat.fields) {
        _controllers[field.key] = TextEditingController(
          text: _mgr.activeProfile.get(field.key),
        );
      }
    }
  }

  void _selectProfile(String name) {
    // Find in presets first, then customs.
    final preset =
        IdentityProfile.presets.where((p) => p.name == name).firstOrNull;
    if (preset != null) {
      _mgr.selectPreset(name);
    } else {
      _mgr.selectCustom(name);
    }

    setState(() {
      _selectedProfileName = _mgr.activeProfile.name;
      _buildControllers();
    });
    widget.onChanged?.call();
  }

  void _onFieldChanged(String key, String value) {
    _mgr.activeProfile.set(key, value);
    // Defer save to avoid writing on every keystroke.
  }

  void _saveEdits() {
    // Flush all controller values into the profile.
    for (final cat in profileFieldDefs) {
      for (final field in cat.fields) {
        final value = _controllers[field.key]?.text ?? '';
        _mgr.activeProfile.set(field.key, value);
      }
    }
    _mgr.save();
    widget.onChanged?.call();
  }

  void _cloneAsCustom() {
    final nameController =
        TextEditingController(text: '${_mgr.activeProfile.name} (Custom)');
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Save as Custom Profile'),
        content: TextField(
          controller: nameController,
          decoration: const InputDecoration(
            labelText: 'Profile name',
            border: OutlineInputBorder(),
          ),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              final name = nameController.text.trim();
              if (name.isEmpty) return;
              // Flush current edits first.
              _saveEdits();
              _mgr.saveAsCustom(name);
              Navigator.pop(ctx);
              setState(() {
                _selectedProfileName = name;
                _buildControllers();
              });
              widget.onChanged?.call();
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
    nameController.dispose;
  }

  void _deleteCustom() {
    if (_mgr.activeProfile.isPreset) return;
    final name = _mgr.activeProfile.name;
    _mgr.deleteCustom(name);
    setState(() {
      _selectedProfileName = _mgr.activeProfile.name;
      _buildControllers();
    });
    widget.onChanged?.call();
  }

  @override
  Widget build(BuildContext context) {
    final allProfiles = _mgr.allProfiles;
    final isCustom = !_mgr.activeProfile.isPreset &&
        _mgr.activeProfile.fields.isNotEmpty;

    return Dialog(
      insetPadding: const EdgeInsets.all(24),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 800, maxHeight: 700),
        child: Column(
          children: [
            // ── Header ──
            Container(
              padding: const EdgeInsets.fromLTRB(20, 16, 12, 12),
              decoration: BoxDecoration(
                color: const Color(0xFFF0F4FF),
                borderRadius:
                    const BorderRadius.vertical(top: Radius.circular(12)),
                border: Border(
                  bottom: BorderSide(color: Colors.grey.shade300),
                ),
              ),
              child: Row(
                children: [
                  const Icon(Icons.fingerprint, size: 22, color: Color(0xFF3F51B5)),
                  const SizedBox(width: 8),
                  const Text(
                    'Identity Profile Editor',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                  const Spacer(),
                  IconButton(
                    icon: const Icon(Icons.close, size: 20),
                    onPressed: () {
                      _saveEdits();
                      Navigator.pop(context);
                    },
                    tooltip: 'Close',
                  ),
                ],
              ),
            ),

            // ── Profile selector ──
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              color: const Color(0xFFFAFAFA),
              child: Row(
                children: [
                  const Text('Profile: ',
                      style: TextStyle(fontWeight: FontWeight.w500)),
                  const SizedBox(width: 8),
                  Expanded(
                    child: DropdownButtonFormField<String>(
                      initialValue: allProfiles
                              .any((p) => p.name == _selectedProfileName)
                          ? _selectedProfileName
                          : allProfiles.first.name,
                      isExpanded: true,
                      decoration: const InputDecoration(
                        isDense: true,
                        contentPadding:
                            EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                        border: OutlineInputBorder(),
                      ),
                      items: [
                        // Presets header.
                        const DropdownMenuItem<String>(
                          enabled: false,
                          child: Text('— Presets —',
                              style: TextStyle(
                                  color: Colors.grey,
                                  fontSize: 12,
                                  fontWeight: FontWeight.w600)),
                        ),
                        ...IdentityProfile.presets.map((p) =>
                            DropdownMenuItem(
                                value: p.name,
                                child: Text(p.name,
                                    style: const TextStyle(fontSize: 13)))),
                        if (_mgr.customProfiles.isNotEmpty) ...[
                          const DropdownMenuItem<String>(
                            enabled: false,
                            child: Text('— Custom —',
                                style: TextStyle(
                                    color: Colors.grey,
                                    fontSize: 12,
                                    fontWeight: FontWeight.w600)),
                          ),
                          ..._mgr.customProfiles.map((p) =>
                              DropdownMenuItem(
                                  value: p.name,
                                  child: Text(p.name,
                                      style: const TextStyle(fontSize: 13)))),
                        ],
                      ],
                      onChanged: (v) {
                        if (v != null) _selectProfile(v);
                      },
                    ),
                  ),
                  const SizedBox(width: 8),
                  Tooltip(
                    message: 'Clone as custom profile',
                    child: IconButton(
                      icon: const Icon(Icons.copy, size: 18),
                      onPressed: _cloneAsCustom,
                    ),
                  ),
                  if (isCustom)
                    Tooltip(
                      message: 'Delete custom profile',
                      child: IconButton(
                        icon: Icon(Icons.delete_outline,
                            size: 18, color: Colors.red.shade400),
                        onPressed: _deleteCustom,
                      ),
                    ),
                ],
              ),
            ),

            // ── Description ──
            if (_mgr.activeProfile.description.isNotEmpty)
              Container(
                width: double.infinity,
                padding:
                    const EdgeInsets.symmetric(horizontal: 20, vertical: 6),
                color: const Color(0xFFFAFAFA),
                child: Text(
                  _mgr.activeProfile.description,
                  style: TextStyle(fontSize: 12, color: Colors.grey.shade600),
                ),
              ),

            // ── Field categories ──
            Expanded(
              child: _mgr.activeProfile.fields.isEmpty
                  ? Center(
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Icon(Icons.visibility_off,
                              size: 48, color: Colors.grey.shade300),
                          const SizedBox(height: 12),
                          Text(
                            'No identity spoofing active',
                            style: TextStyle(
                                fontSize: 15, color: Colors.grey.shade500),
                          ),
                          const SizedBox(height: 4),
                          Text(
                            'Select a profile above to start spoofing',
                            style: TextStyle(
                                fontSize: 12, color: Colors.grey.shade400),
                          ),
                        ],
                      ),
                    )
                  : ListView.builder(
                      padding: const EdgeInsets.only(bottom: 12),
                      itemCount: profileFieldDefs.length,
                      itemBuilder: (context, index) {
                        final cat = profileFieldDefs[index];
                        final isExpanded =
                            _expandedCategories.contains(cat.name);
                        return _buildCategory(cat, isExpanded);
                      },
                    ),
            ),

            // ── Footer ──
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              decoration: BoxDecoration(
                color: const Color(0xFFF5F5F5),
                border: Border(
                  top: BorderSide(color: Colors.grey.shade300),
                ),
              ),
              child: Row(
                children: [
                  Text(
                    '${_mgr.activeProfile.fields.length} fields configured',
                    style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
                  ),
                  const Spacer(),
                  TextButton(
                    onPressed: () {
                      // Reset to preset defaults.
                      _selectProfile(_selectedProfileName);
                    },
                    child: const Text('Reset'),
                  ),
                  const SizedBox(width: 8),
                  FilledButton(
                    onPressed: () {
                      _saveEdits();
                      Navigator.pop(context);
                    },
                    child: const Text('Apply & Close'),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildCategory(CategoryDef cat, bool isExpanded) {
    // Count non-empty fields in this category.
    int filledCount = 0;
    for (final f in cat.fields) {
      if (_mgr.activeProfile.get(f.key).isNotEmpty) filledCount++;
    }

    return Column(
      children: [
        InkWell(
          onTap: () {
            setState(() {
              if (isExpanded) {
                _expandedCategories.remove(cat.name);
              } else {
                _expandedCategories.add(cat.name);
              }
            });
          },
          child: Container(
            padding:
                const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
            decoration: BoxDecoration(
              color: isExpanded
                  ? const Color(0xFFE8EAF6)
                  : Colors.white,
              border: Border(
                bottom: BorderSide(color: Colors.grey.shade200, width: 0.5),
              ),
            ),
            child: Row(
              children: [
                Icon(
                  isExpanded ? Icons.expand_more : Icons.chevron_right,
                  size: 20,
                  color: Colors.grey.shade600,
                ),
                const SizedBox(width: 8),
                Text(
                  cat.name,
                  style: const TextStyle(
                    fontSize: 13,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(width: 8),
                Text(
                  '$filledCount / ${cat.fields.length}',
                  style: TextStyle(fontSize: 11, color: Colors.grey.shade500),
                ),
              ],
            ),
          ),
        ),
        if (isExpanded)
          Container(
            color: const Color(0xFFFAFAFA),
            padding:
                const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
            child: Column(
              children: cat.fields
                  .map((f) => _buildField(f))
                  .toList(),
            ),
          ),
      ],
    );
  }

  Widget _buildField(FieldDef field) {
    final controller = _controllers[field.key]!;

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 180,
            child: Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    field.label,
                    style: const TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                  if (field.hint.isNotEmpty)
                    Text(
                      field.hint,
                      style: TextStyle(
                        fontSize: 10,
                        color: Colors.grey.shade500,
                      ),
                    ),
                ],
              ),
            ),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: TextField(
              controller: controller,
              style: const TextStyle(fontSize: 12, fontFamily: 'monospace'),
              maxLines: null,
              decoration: InputDecoration(
                isDense: true,
                contentPadding:
                    const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                border: const OutlineInputBorder(),
                enabledBorder: OutlineInputBorder(
                  borderSide: BorderSide(color: Colors.grey.shade300),
                ),
                hintText: field.hint,
                hintStyle: TextStyle(
                  fontSize: 11,
                  color: Colors.grey.shade400,
                  fontFamily: 'monospace',
                ),
              ),
              onChanged: (v) => _onFieldChanged(field.key, v),
            ),
          ),
        ],
      ),
    );
  }
}

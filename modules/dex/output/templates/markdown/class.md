
# {{ class.name }} Class

{% if class.documentation.brief %}
**Brief:** {{ class.documentation.brief }}
{% endif %}

## Detailed description

{{ class.documentation.description }}

## Members documentation

{% for f in class.members | filter_by_type: 'function' %}

### {{ f.name }}

{% if f.documentation.brief %}
**Brief:** {{ f.documentation.brief }}
{% endif %}

{% if f.parameters and f.parameters.size > 0 %}
Parameters:
{% for p in f.parameters %}
- {{ p.documentation }}
{% endfor %}
{% endif %}

{% if f.documentation.returns %}
**Returns:** {{ f.documentation.returns }}
{% endif %}

{{ f.documentation.description }}

{% endfor %}

{% assign non_members = class | related_non_members %}
{% if non_members.length > 0 %}
## Non-Members documentation

{% for f in non_members %}

### {{ f.name }}

{% if f.documentation.brief %}
**Brief:** {{ f.documentation.brief }}
{% endif %}

{% if f.parameters and f.parameters.size > 0 %}
Parameters:
{% for p in f.parameters %}
- {{ p.documentation }}{% newline %}
{% endfor %}
{% endif %}

{% if f.documentation.returns %}
**Returns:** {{ f.documentation.returns }}
{% endif %}

{{ f.documentation.description }}

{% endfor %}

{% endif %}